//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./typechecker.h"
#include "../ast/visitors.h"
#include "../errors/reporter.h"
#include "../utility/flags.h"
#include "../utility/pretty.h"
#include "./environment.h"
#include "./name_resolver.h"
#include "absl/container/flat_hash_map.h"
#include "llvm/Target/TargetMachine.h"
#include <stack>
#include <vector>

namespace ast = gal::ast;
namespace internal = gal::internal;

namespace {
  using DT = ast::DeclType;
  using ET = ast::ExprType;
  using TT = ast::TypeType;
  using ST = ast::StmtType;

  std::unique_ptr<ast::Type> uint_type(ast::SourceLoc loc, int width) noexcept {
    return std::make_unique<ast::BuiltinIntegralType>(std::move(loc), false, static_cast<ast::IntegerWidth>(width));
  }

  std::unique_ptr<ast::Type> slice_of(ast::SourceLoc loc, std::unique_ptr<ast::Type> type, bool mut) noexcept {
    return std::make_unique<ast::SliceType>(std::move(loc), mut, std::move(type));
  }

  std::unique_ptr<ast::Type> bool_type(ast::SourceLoc loc) noexcept {
    return std::make_unique<ast::BuiltinBoolType>(std::move(loc));
  }

  std::unique_ptr<ast::Type> void_type(ast::SourceLoc loc) noexcept {
    return std::make_unique<ast::VoidType>(std::move(loc));
  }

  std::unique_ptr<ast::Type> byte_type(ast::SourceLoc loc) noexcept {
    return std::make_unique<ast::BuiltinByteType>(std::move(loc));
  }

  std::unique_ptr<ast::Type> fn_pointer_for(ast::SourceLoc loc, const ast::FnPrototype& proto) noexcept {
    auto args = std::vector<std::unique_ptr<ast::Type>>{};
    auto proto_args = proto.args();

    absl::c_transform(proto_args, std::back_inserter(args), [](const ast::Argument& argument) {
      return argument.type().clone();
    });

    return std::make_unique<ast::FnPointerType>(std::move(loc), std::move(args), proto.return_type().clone());
  }

  GALLIUM_COLD std::unique_ptr<ast::Type> error_type() noexcept {
    return std::make_unique<ast::ErrorType>();
  }

  GALLIUM_COLD gal::PointedOut type_was(const ast::Expression& expr,
      gal::DiagnosticType type,
      std::string_view msg_prefix = "") noexcept {
    auto msg = absl::StrCat(msg_prefix, "real type was `", gal::to_string(expr.result()), "`");

    return gal::point_out_part(expr, type, std::move(msg));
  }

  GALLIUM_COLD gal::PointedOut type_was_err(const ast::Expression& expr, std::string_view msg_prefix = "") noexcept {
    return type_was(expr, gal::DiagnosticType::error, msg_prefix);
  }

  GALLIUM_COLD gal::PointedOut type_was_note(const ast::Expression& expr, std::string_view msg_prefix = "") noexcept {
    return type_was(expr, gal::DiagnosticType::note, msg_prefix);
  }

  GALLIUM_COLD gal::PointedOut expected_type(const ast::Type& type) noexcept {
    auto msg = absl::StrCat("expected type `", gal::to_string(type), "`");

    return gal::point_out_part(type, gal::DiagnosticType::note, std::move(msg));
  }

  static ast::PointerType mut_byte_ptr{ast::SourceLoc::nonexistent(), true, byte_type(ast::SourceLoc::nonexistent())};

  static ast::PointerType byte_ptr{ast::SourceLoc::nonexistent(), false, byte_type(ast::SourceLoc::nonexistent())};

  static ast::BuiltinIntegralType default_int{ast::SourceLoc::nonexistent(), true, static_cast<ast::IntegerWidth>(64)};

  static ast::BuiltinIntegralType ptr_width_int{ast::SourceLoc::nonexistent(), true, ast::IntegerWidth::native_width};

  class TypeChecker final : public ast::AnyVisitorBase<void, ast::Type*> {
    using Expr = ast::ExpressionVisitor<ast::Type*>;
    using Base = ast::AnyVisitorBase<void, ast::Type*>;

  public:
    explicit TypeChecker(ast::Program* program,
        const llvm::TargetMachine& machine,
        gal::DiagnosticReporter* reporter) noexcept
        : program_{program},
          diagnostics_{reporter},
          resolver_{program_, diagnostics_},
          machine_{machine},
          layout_{machine.createDataLayout()} {}

    bool type_check() noexcept {
      if (diagnostics_->had_error()) {
        return false;
      }

      Base::walk_ast(program_);

      return !diagnostics_->had_error();
    }

    void visit(ast::ImportDeclaration*) final {}

    void visit(ast::ImportFromDeclaration*) final {}

    void visit(ast::FnDeclaration* decl) final {
      resolver_.enter_scope();
      expected_ = decl->proto_mut()->return_type_mut();

      for (auto& arg : decl->proto_mut()->args_mut()) {
        resolver_.add_local(arg.name(), gal::ScopeEntity{arg.loc(), arg.type_mut(), false});
      }

      visit_children(decl);
      resolver_.leave_scope();

      // we can safely ignore any checks if it's void
      if (expected_->is(TT::builtin_void)) {
        return;
      }

      if (identical(*expected_, decl->body())) {
        return;
      }

      // if we can do an implicit conversion on the **last** block member to fix type errors
      // we do it here, and then early return
      if (!decl->body().statements().empty()) {
        if (auto& back = decl->body_mut()->statements_mut().back(); back->is(ST::expr)) {
          auto* expr = gal::as_mut<ast::ExpressionStatement>(back.get());

          if (try_make_compatible(*expected_, expr->expr_owner())) {
            decl->body_mut()->result_update(expr->expr().result().clone());

            return Expr::return_value(expr->expr_mut()->result_mut());
          }
        }
      }

      auto a = gal::point_out_part(*expected_, gal::DiagnosticType::note, "return type was here");
      auto list = gal::into_list(std::move(a));

      if (decl->body().statements().empty()) {
        list.push_back(type_was_err(decl->body()));

      } else {
        auto& front = *decl->body().statements().back();

        if (front.is(ST::expr)) {
          auto& expr_stmt = gal::as<ast::ExpressionStatement>(front);

          list.push_back(type_was_err(expr_stmt.expr()));
        } else {
          list.push_back(gal::point_out_part(front.loc(), gal::DiagnosticType::note, "type was `void`"));
        }
      }

      diagnostics_->report_emplace(31, gal::into_list(gal::point_out_list(std::move(list))));
    }

    void visit(ast::StructDeclaration* decl) final {
      visit_children(decl);
    }

    void visit(ast::ClassDeclaration*) final {}

    void visit(ast::TypeDeclaration*) final {}

    void visit(ast::MethodDeclaration*) final {}

    void visit(ast::ExternalFnDeclaration*) final {}

    void visit(ast::ExternalDeclaration*) final {}

    void visit(ast::ConstantDeclaration* decl) final {
      constant_only_ = true;
      visit_children(decl);
      constant_only_ = false;
    }

    void visit(ast::BindingStatement* stmt) final {
      visit_children(stmt);
      auto& expr_type = stmt->initializer().result();

      if (auto hint = stmt->hint()) {
        if (!try_make_compatible(**hint, stmt->initializer_owner())) {
          auto a = type_was_err(stmt->initializer());
          auto b = expected_type(**hint);
          auto c = gal::point_out_list(std::move(a), std::move(b));

          diagnostics_->report_emplace(7, gal::into_list(std::move(c)));
        }

        // need to base it off of the binding's type if possible, e.g binding is ptr type and init is `nil`
        resolver_.add_local(stmt->name(), gal::ScopeEntity{stmt->loc(), *stmt->hint_mut(), stmt->mut()});
      } else {
        if (expr_type.is(TT::nil_pointer)) {
          auto a = gal::point_out_list(type_was_err(stmt->initializer()));
          auto b = gal::single_message("help: try casting to `*const byte`");

          diagnostics_->report_emplace(21, gal::into_list(std::move(a), std::move(b)));
        }

        convert_intermediate(stmt->initializer_owner());

        resolver_.add_local(stmt->name(),
            gal::ScopeEntity{stmt->loc(), stmt->initializer_mut()->result_mut(), stmt->mut()});
      }
    }

    void visit(ast::ExpressionStatement* stmt) final {
      visit_children(stmt);
    }

    void visit(ast::AssertStatement* stmt) final {
      visit_children(stmt);
    }

    void visit(ast::StringLiteralExpression* expr) final {
      update_return(expr, slice_of(expr->loc(), uint_type(expr->loc(), 8), false));
    }

    void visit(ast::IntegerLiteralExpression* expr) final {
      update_return(expr, std::make_unique<ast::UnsizedIntegerType>(expr->loc(), expr->value()));
    }

    void visit(ast::FloatLiteralExpression* expr) final {
      update_return(expr, std::make_unique<ast::BuiltinFloatType>(expr->loc(), ast::FloatWidth::ieee_double));
    }

    void visit(ast::BoolLiteralExpression* expr) final {
      update_return(expr, bool_type(expr->loc()));
    }

    void visit(ast::CharLiteralExpression* expr) final {
      update_return(expr, uint_type(expr->loc(), 8));
    }

    void visit(ast::NilLiteralExpression* expr) final {
      update_return(expr, std::make_unique<ast::NilPointerType>(expr->loc()));
    }

    void visit(ast::LocalIdentifierExpression* expr) final {
      if (!constant_only_) {
        if (auto local = resolver_.local(expr->name())) {
          return update_return(expr, (*local)->type().clone());
        }
      }

      if (auto qualified = resolver_.qualified_for(ast::UnqualifiedID{std::nullopt, std::string{expr->name()}})) {
        auto& [id, _] = *qualified;

        if (check_qualified_id(self_expr_owner(), id)) {
          return;
        }
      }

      auto a = report_unknown_entity(*expr);

      diagnostics_->report_emplace(18, gal::into_list(std::move(a)));

      update_return(expr, error_type());
    }

    GALLIUM_COLD std::unique_ptr<gal::DiagnosticPart> report_unknown_entity(
        const ast::LocalIdentifierExpression& expr) noexcept {
      auto vec = std::vector<gal::PointedOut>{};

      vec.push_back(gal::point_out_part(expr, gal::DiagnosticType::error, "usage was here"));

      if (auto qualified = resolver_.qualified_for(ast::UnqualifiedID{std::nullopt, std::string{expr.name()}})) {
        auto& [id, _] = *qualified;

        if (auto entity = resolver_.entity(id)) {
          vec.push_back(
              gal::point_out_part((*entity)->decl().loc(), gal::DiagnosticType::note, "name referred to this"));
        }
      }

      return gal::point_out_list(std::move(vec));
    }

    void visit(ast::IdentifierExpression* expr) final {
      auto id = expr->id();

      if (!check_qualified_id(self_expr_owner(), id)) {
        auto entity = resolver_.entity(id);
        assert(entity.has_value());
        auto a = gal::point_out_part(*expr, gal::DiagnosticType::error, "usage was here");
        auto b = gal::point_out_part((*entity)->decl(), gal::DiagnosticType::note, "decl referred to was here");

        diagnostics_->report_emplace(22, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));

        update_return(expr, error_type());
      }
    }

    void visit(ast::StructExpression* expr) final {
      visit_children(expr);

      // if the type that the user was trying to make a struct instance of
      // isn't actually a struct type, just bail out immediately
      if (!expr->struct_type().is(TT::user_defined)) {
        auto error = gal::point_out(expr->struct_type(), gal::DiagnosticType::error);

        diagnostics_->report_emplace(10, gal::into_list(std::move(error)));

        return update_return(expr, error_type());
      }

      auto& type = gal::as<ast::UserDefinedType>(expr->struct_type());

      if (auto decl = resolver_.struct_type(type.id())) {
        for (auto& s_field : (*decl)->fields()) {
          auto init_fields = expr->fields_mut();
          auto init_field = absl::c_find_if(init_fields, [&](ast::FieldInitializer& field) {
            return field.name() == s_field.name();
          });

          if (init_field == init_fields.end()) {
            auto a = gal::point_out_part(s_field.loc(), gal::DiagnosticType::note, "field declared here");
            auto b = gal::point_out_part(*expr,
                gal::DiagnosticType::error,
                absl::StrCat("struct-init was here, missing field `", s_field.name(), "`"));

            diagnostics_->report_emplace(12, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));

            break;
          }

          if (!try_make_compatible(s_field.type(), init_field->init_owner())) {
            auto a = gal::point_out_part(init_field->init(),
                gal::DiagnosticType::error,
                absl::StrCat("expr evaluated to `", gal::to_string(init_field->init().result()), "`"));

            auto b = gal::point_out_part(s_field.loc(),
                gal::DiagnosticType::note,
                absl::StrCat("expected type `", gal::to_string(s_field.type()), "`"));

            diagnostics_->report_emplace(13, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
          }
        }

        update_return(expr, expr->struct_type().clone());
      } else {
        update_return(expr, error_type());
      }
    }

    void visit(ast::StaticGlobalExpression*) final {
      assert(false);
    }

    void visit(ast::ArrayExpression* expr) final {
      visit_children(expr);

      auto elements = expr->elements_mut();
      auto it = std::find_if(elements.begin() + 1, elements.end(), [&](const std::unique_ptr<ast::Expression>& expr) {
        return expr->result() != elements.front()->result();
      });

      // array is guaranteed by the syntax to be larger than 0
      if (it == elements.end()) {
        for (auto& element : elements) {
          convert_intermediate(&element);
        }

        update_return(expr,
            std::make_unique<ast::ArrayType>(expr->loc(), elements.size(), elements.front()->result().clone()));
      } else {
        auto a = type_was_err(*elements.front());
        auto b = type_was_note(**it);

        diagnostics_->report_emplace(34, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));

        update_return(expr, error_type());
      }
    }

    void visit(ast::CallExpression* expr) final {
      // have to do weird stuff with ownership here, since we
      // aren't using the visit_children API
      auto* self = Base::self_expr_owner();

      for (auto& arg : expr->args_mut()) {
        Base::accept(&arg);
      }

      // if it's not an identifier, it could be an identifier-local that gets translated to an identifier
      if (!expr->callee().is(ET::identifier)) {
        ignore_ambiguous_fn_ref_ = true;
        Base::accept(expr->callee_owner());
        ignore_ambiguous_fn_ref_ = false;
      }

      // we need to do special handling here in order to not break over function overloading n whatnot
      if (expr->callee().is(ET::identifier)) {
        auto& identifier = gal::as<ast::IdentifierExpression>(expr->callee());

        if (auto overloads = resolver_.overloads(identifier.id())) {
          if (auto overload = select_overload(*expr, **overloads, expr->args_mut())) {
            *self = ast::StaticCallExpression::from_call(identifier.id(), **overload, expr);

            return update_return(self->get(), (*overload)->proto().return_type().clone());
          } else {
            return update_return(expr, error_type());
          }
        }

        auto a = gal::point_out(*expr, gal::DiagnosticType::error, "usage was here");
        diagnostics_->report_emplace(28, gal::into_list(std::move(a)));

        return update_return(expr, error_type());
      }

      // we picked up on one single overload, but it got resolved earlier
      if (expr->callee().result().is(TT::fn_pointer)) {
        auto& callee = expr->callee();
        auto& fn_ptr_type = gal::as<ast::FnPointerType>(callee.result());

        if (callable(fn_ptr_type.args(), expr->args_mut())) {
          return update_return(expr, fn_ptr_type.return_type().clone());
        }

        if (expr->callee().is(ET::static_global)) {
          auto& global = gal::as<ast::StaticGlobalExpression>(expr->callee());
          report_uncallable(global.decl().loc(), callee, fn_ptr_type.args(), expr->args_mut());
        } else {
          report_uncallable(callee.loc(), callee, fn_ptr_type.args(), expr->args_mut());
        }
      } else {
        auto a = gal::point_out_list(type_was_err(expr->callee()));

        diagnostics_->report_emplace(30, gal::into_list(std::move(a)));
      }

      return update_return(expr, error_type());
    }

    void visit(ast::StaticCallExpression*) final {
      assert(false);
    }

    void visit(ast::MethodCallExpression*) final {}

    void visit(ast::StaticMethodCallExpression*) final {}

    void visit(ast::IndexExpression* expr) final {
      visit_children(expr);

      // may get "indirected" by the auto-deref code
      // and then fail, in which case we still need access to it
      auto* callee = expr->callee_mut();

      if (expr->callee().result().is(TT::reference)) {
        auto_deref(expr->callee_owner());
      }

      // while we want to get it down to just ids, ptrs and refs, we don't want to allow
      // raw indexing into pointers without a `*`
      if (is_indirection_to(TT::slice, expr->callee()) || is_indirection_to(TT::array, expr->callee())) {
        unwrap_indirection(expr->callee_owner());
      } else if (!expr->callee().result().is_one_of(TT::slice, TT::array)) {
        auto a = type_was_err(*callee);
        auto b = gal::point_out_part(*callee, gal::DiagnosticType::note, "tried to index here");
        auto c = gal::point_out_list(std::move(a), std::move(b));

        diagnostics_->report_emplace(46, gal::into_list(std::move(c)));

        return update_return(expr, error_type());
      }

      auto args = expr->indices_mut();

      // there is infrastructure for multi-dimensional array
      // lookups, but it doesn't do anything right now
      if (args.size() != 1) {
        auto b = gal::point_out(*expr, gal::DiagnosticType::error, "must have exactly one number in `[]`s");

        diagnostics_->report_emplace(47, gal::into_list(std::move(b)));

        return update_return(expr, error_type());
      } else if (!try_make_compatible(ptr_width_int, &args.front())) {
        auto a = type_was_note(*args.front());
        auto b =
            gal::point_out_part(*expr, gal::DiagnosticType::error, "cannot index into array a type other than `isize`");

        diagnostics_->report_emplace(48, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));

        return update_return(expr, error_type());
      }

      return update_return(expr,
          std::make_unique<ast::IndirectionType>(expr->loc(),
              array_type(expr->callee_mut()->result_mut())->clone(),
              mut(expr->callee())));
    }

    [[nodiscard]] const ast::Type& accessed_type(const ast::Type& type) noexcept {
      switch (type.type()) {
        case TT::pointer: {
          auto& p = gal::as<ast::PointerType>(type);

          return p.pointed();
        }
        case TT::reference: {
          auto& r = gal::as<ast::ReferenceType>(type);

          return r.referenced();
        }
        case TT::indirection: {
          auto& i = gal::as<ast::IndirectionType>(type);

          return i.produced();
        }
        default: return type;
      }
    }

    void visit(ast::FieldAccessExpression* expr) final {
      visit_children(expr);

      if (expr->object().result().is(TT::reference)) {
        auto_deref(expr->object_owner());
      }

      if (is_indirection_to(TT::user_defined, expr->object())) {
        unwrap_indirection(expr->object_owner());
      } else if (!expr->object().result().is(TT::user_defined)) {
        auto a = gal::point_out_list(type_was_err(expr->object()));
        auto b = gal::single_message(absl::StrCat("cannot access field `",
            expr->field_name(),
            "` on type `",
            gal::to_string(expr->object().result()),
            "`"));

        diagnostics_->report_emplace(35, gal::into_list(std::move(a), std::move(b)));

        return update_return(expr, error_type());
      }

      auto& held = accessed_type(expr->object().result());
      auto& type = gal::as<ast::UserDefinedType>(held);

      if (auto struct_type = resolver_.struct_type(type.id())) {
        auto& decl = **struct_type;
        auto fields = decl.fields();
        auto found = std::find_if(fields.begin(), fields.end(), [expr](const ast::Field& field) {
          return field.name() == expr->field_name();
        });

        if (found == fields.end()) {
          auto a = gal::point_out_part(*expr, gal::DiagnosticType::error, "field was here");
          auto b = gal::point_out_part(decl, gal::DiagnosticType::note, "referred-to type was here");
          auto c = gal::single_message(
              absl::StrCat("cannot access field `", expr->field_name(), "` on type `", gal::to_string(type), "`"));

          diagnostics_->report_emplace(35,
              gal::into_list(gal::point_out_list(std::move(a), std::move(b)), std::move(c)));

          return update_return(expr, error_type());
        }

        return update_return(expr,
            std::make_unique<ast::IndirectionType>(expr->loc(), found->type().clone(), mut(expr->object())));
      } else {
        assert(false);
      }
    }

    void visit(ast::GroupExpression* expr) final {
      visit_children(expr);

      update_return(expr, expr->expr().result().clone());
    }

    void visit(ast::UnaryExpression* expr) final {
      visit_children(expr);

      switch (expr->op()) {
        case ast::UnaryOp::logical_not: {
          if (!boolean(expr->expr())) {
            auto a = type_was_note(expr->expr());
            auto b = gal::point_out_part(*expr, gal::DiagnosticType::error, "`not` can only operate on booleans");

            diagnostics_->report_emplace(38, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));

            return update_return(expr, error_type());
          } else {
            return update_return(expr, bool_type(expr->loc()));
          }
        }
        case ast::UnaryOp::negate:
        case ast::UnaryOp::bitwise_not: {
          if (!integral(expr->expr())) {
            auto a = type_was_note(expr->expr());
            auto b = gal::point_out_part(*expr, gal::DiagnosticType::error, "expr was not integral");
            auto c = gal::point_out_list(std::move(a), std::move(b));
            auto d = gal::single_message(
                absl::StrCat("operator `", gal::unary_op_string(expr->op()), "` must have an integral type"));

            diagnostics_->report_emplace(39, gal::into_list(std::move(c), std::move(d)));

            return update_return(expr, error_type());
          } else {
            return update_return(expr, expr->expr().result().clone());
          }
        }
        case ast::UnaryOp::ref_to:
        case ast::UnaryOp::mut_ref_to: {
          if (!lvalue(expr->expr())) {
            auto a = type_was_note(expr->expr());
            auto b = gal::point_out_part(*expr, gal::DiagnosticType::error, "expr was not an lvalue");
            auto c = gal::point_out_list(std::move(a), std::move(b));
            auto d = gal::single_message(
                absl::StrCat("operator `", gal::unary_op_string(expr->op()), "` must have an lvalue expression"));

            diagnostics_->report_emplace(43, gal::into_list(std::move(c), std::move(d)));

            return update_return(expr, error_type());
          }

          if (expr->op() == ast::UnaryOp::mut_ref_to && !mut(expr->expr())) {
            auto a = report_not_mut(expr->expr());
            auto b = gal::point_out_part(*expr, gal::DiagnosticType::error, "cannot take ref to non-`mut` object");
            auto c = gal::point_out_list(std::move(a), std::move(b));

            diagnostics_->report_emplace(44, gal::into_list(std::move(c)));

            return update_return(expr, error_type());
          }

          auto type = expr->expr().result().clone();
          auto op = expr->op();
          replace_self(std::make_unique<ast::AddressOfExpression>(expr->loc(), std::move(*expr->expr_owner())));
          auto* self = self_expr();

          return update_return(self,
              std::make_unique<ast::ReferenceType>(self->loc(), op == ast::UnaryOp::mut_ref_to, std::move(type)));
        }
        case ast::UnaryOp::dereference: {
          auto& type = expr->expr().result();

          switch (type.type()) {
            case TT::pointer: {
              auto& ptr = gal::as<ast::PointerType>(type);

              return update_return(expr,
                  std::make_unique<ast::IndirectionType>(expr->loc(), ptr.pointed().clone(), mut(expr->expr())));
            }
            case TT::reference: {
              auto& ref = gal::as<ast::ReferenceType>(type);

              return update_return(expr,
                  std::make_unique<ast::IndirectionType>(expr->loc(), ref.referenced().clone(), mut(expr->expr())));
            }
            case TT::indirection: {
              auto& indirection = gal::as<ast::IndirectionType>(type);

              if (indirection.produced().is(TT::reference)) {
                auto& ref = gal::as<ast::ReferenceType>(indirection.produced());

                *expr->expr_owner() =
                    std::make_unique<ast::LoadExpression>(expr->expr().loc(), std::move(*expr->expr_owner()));
                expr->expr_mut()->result_update(ref.clone());

                return update_return(expr,
                    std::make_unique<ast::IndirectionType>(expr->loc(), ref.referenced().clone(), mut(expr->expr())));
              } else if (indirection.produced().is(TT::pointer)) {
                auto& ptr = gal::as<ast::PointerType>(indirection.produced());

                *expr->expr_owner() =
                    std::make_unique<ast::LoadExpression>(expr->expr().loc(), std::move(*expr->expr_owner()));
                expr->expr_mut()->result_update(ptr.clone());

                return update_return(expr,
                    std::make_unique<ast::IndirectionType>(expr->loc(), ptr.pointed().clone(), mut(expr->expr())));
              } else {
                [[fallthrough]];
              }
            }
            default: {
              auto a = type_was_note(expr->expr());
              auto b = gal::point_out_part(*expr,
                  gal::DiagnosticType::error,
                  absl::StrCat("cannot dereference expression of type `", gal::to_string(expr->expr().result()), "`"));

              auto c = gal::point_out_list(std::move(a), std::move(b));
              diagnostics_->report_emplace(45, gal::into_list(std::move(c)));

              return update_return(expr, error_type());
            }
          }
        }
        default: assert(false); break;
      }
    }

    void visit(ast::BinaryExpression* expr) final {
      visit_children(expr);

      switch (expr->op()) {
        case ast::BinaryOp::mul:
        case ast::BinaryOp::div:
        case ast::BinaryOp::mod:
        case ast::BinaryOp::add:
        case ast::BinaryOp::sub: {
          if (check_binary_conditions(expr, arithmetic, 39, "arithmetic")) {
            return update_return(expr, expr->lhs().result().clone());
          }

          break;
        }
        case ast::BinaryOp::left_shift:
        case ast::BinaryOp::right_shift:
        case ast::BinaryOp::bitwise_and:
        case ast::BinaryOp::bitwise_or:
        case ast::BinaryOp::bitwise_xor: {
          if (check_binary_conditions(expr, integral, 41, "integral")) {
            return update_return(expr, expr->lhs().result().clone());
          }

          break;
        }
        case ast::BinaryOp::lt:
        case ast::BinaryOp::gt:
        case ast::BinaryOp::lt_eq:
        case ast::BinaryOp::gt_eq: {
          if (!check_condition(expr, arithmetic, 39, "arithmetic")) {
            return;
          }

          [[fallthrough]];
        }
        case ast::BinaryOp::equals:
        case ast::BinaryOp::not_equal: {
          if (check_identical(expr)) {
            return update_return(expr, bool_type(expr->loc()));
          }

          break;
        }
        case ast::BinaryOp::logical_and:
        case ast::BinaryOp::logical_or:
        case ast::BinaryOp::logical_xor: {
          if (check_binary_conditions(expr, boolean, 38, "boolean")) {
            return update_return(expr, bool_type(expr->loc()));
          }

          break;
        }
        case ast::BinaryOp::left_shift_eq:
        case ast::BinaryOp::right_shift_eq:
        case ast::BinaryOp::bitwise_and_eq:
        case ast::BinaryOp::bitwise_or_eq:
        case ast::BinaryOp::bitwise_xor_eq: {
          if (!check_condition(expr, integral, 41, "integral")) {
            return update_return(expr, error_type());
          }

          [[fallthrough]];
        }
        case ast::BinaryOp::add_eq:
        case ast::BinaryOp::sub_eq:
        case ast::BinaryOp::mul_eq:
        case ast::BinaryOp::div_eq:
        case ast::BinaryOp::mod_eq: {
          if (!check_condition(expr, arithmetic, 41, "integral")) {
            return;
          }

          [[fallthrough]];
        }
        case ast::BinaryOp::assignment: {
          if (!lvalue(expr->lhs())) {
            auto a = gal::point_out_list(type_was_err(expr->lhs()));
            auto b =
                gal::single_message("lvalues are identifiers, and the result of `*expr` on pointers and references");

            diagnostics_->report_emplace(42, gal::into_list(std::move(a), std::move(b)));
          }

          if (!mut(expr->lhs())) {
            auto a =
                gal::point_out(expr->lhs(), gal::DiagnosticType::error, "left-hand side of assignment was not `mut`");
            auto b = gal::single_message("cannot assign to immutable lvalue");

            diagnostics_->report_emplace(49, gal::into_list(std::move(a), std::move(b)));
          }

          if (!try_make_compatible(accessed_type(expr->lhs().result()), expr->rhs_owner())) {
            auto a = type_was_err(expr->rhs());
            auto b = type_was_note(expr->lhs());
            auto c = gal::point_out_list(std::move(a), std::move(b));

            diagnostics_->report_emplace(50, gal::into_list(std::move(c)));
          }

          return update_return(expr, void_type(expr->loc()));
        }
        default: assert(false); break;
      }
    }

    void visit(ast::CastExpression* expr) final {
      visit_children(expr);

      if (convertible(expr->cast_to(), expr->castee())) {
        auto pinned = expr->cast_to().clone();

        // this falls apart if the expected type given gets deleted, like it does when
        // we try to replace `expr`. we need to do it manually, and pin our own `expected`
        // type so that we don't end up referencing a deleted object
        implicit_convert(*pinned, self_expr_owner(), expr->castee_owner());

        return Expr::return_value(self_expr()->result_mut());
      }

      if (!expr->unsafe()) {
        auto& result = expr->castee().result();

        switch (expr->cast_to().type()) {
          case TT::builtin_integral:
          case TT::builtin_float:
          case TT::builtin_bool:
          case TT::builtin_byte:
          case TT::builtin_char: {
            if (result.is_one_of(TT::builtin_integral,
                    TT::builtin_byte,
                    TT::builtin_char,
                    TT::builtin_bool,
                    TT::builtin_float)) {
              return update_return(expr, expr->cast_to().clone());
            } else {
              return cast_error(expr, "builtins can only be cast to other builtin types");
            }
          }
          case TT::builtin_void: {
            return cast_error(expr, "cannot cast anything to `void`");
          }
          case TT::dyn_interface:
          case TT::user_defined: {
            return cast_error(expr, "cannot cast between user-defined types");
          }
          case TT::fn_pointer: {
            if (identical(result, mut_byte_ptr) || identical(result, byte_ptr)) {
              return update_return(expr, expr->cast_to().clone());
            }

            return cast_error(expr,
                "cannot cast any type besides a `byte` pointer to a fn pointer",
                "help: try casting to `*const byte` first");
          }
          case TT::reference:
          case TT::slice:
          case TT::pointer:
          case TT::nil_pointer:
          case TT::error:
          case TT::user_defined_unqualified:
          case TT::dyn_interface_unqualified:
          default: assert(false); break;
        }
      }
    }

    void visit(ast::IfThenExpression* expr) final {
      visit_children(expr);

      if (!boolean(expr->condition())) {
        auto a = gal::point_out_list(type_was_err(expr->condition()));

        diagnostics_->report_emplace(15, gal::into_list(std::move(a)));
      }

      // compatible != identical, say its `if thing then nil else &a`. no accidental
      // and severely underpowered type inference is getting in until this compiler is ready!
      if (!identical(expr->true_branch(), expr->false_branch())) {
        auto a = gal::point_out_part(*expr, gal::DiagnosticType::error);
        auto b = type_was_note(expr->true_branch());
        auto c = type_was_note(expr->false_branch());

        diagnostics_->report_emplace(16, gal::into_list(gal::point_out_list(std::move(a), std::move(b), std::move(c))));

        update_return(expr, error_type());
      } else {
        convert_intermediate(expr->true_branch_owner());

        update_return(expr, expr->true_branch().result().clone());
      }
    }

    void visit(ast::IfElseExpression* expr) final {
      visit_children(expr);

      if (!boolean(expr->condition())) {
        auto a = gal::point_out_list(type_was_err(expr->condition()));

        diagnostics_->report_emplace(15, gal::into_list(std::move(a)));
      }

      auto& type = expr->block().result();
      auto all_same = true;

      for (auto& elif : expr->elif_blocks_mut()) {
        if (!boolean(elif.condition())) {
          auto a = gal::point_out_list(type_was_err(elif.condition()));

          diagnostics_->report_emplace(15, gal::into_list(std::move(a)));
        }

        if (!try_make_compatible(type, elif.block_owner())) {
          all_same = false;
        }
      }

      if (auto else_block = expr->else_block_owner()) {
        auto* block = *else_block;

        if (!try_make_compatible(type, block)) {
          all_same = false;
        }
      }

      if (all_same) {
        update_return(expr, type.clone());
      } else {
        update_return(expr, void_type(expr->loc()));
      }
    }

    void visit(ast::BlockExpression* expr) final {
      resolver_.enter_scope();

      visit_children(expr);

      if (auto stmts = expr->statements(); !stmts.empty() && expr->statements().back()->is(ST::expr)) {
        auto& expr_stmt = gal::as<ast::ExpressionStatement>(*stmts.back());

        update_return(expr, expr_stmt.expr().result().clone());
      } else {
        update_return(expr, void_type(expr->loc()));
      }

      resolver_.leave_scope();
    }

    void visit(ast::LoopExpression* expr) final {
      auto type = std::unique_ptr<ast::Type>{};

      {
        auto _ = BeforeAfterLoop(this, true);
        visit_children(expr);

        if (last_break_type_ != nullptr) {
          type = last_break_type_->clone();
        }
      }

      if (type != nullptr) {
        update_return(expr, std::move(type));
      } else {
        update_return(expr, void_type(expr->loc()));
      }
    }

    void visit(ast::WhileExpression* expr) final {
      {
        auto _ = BeforeAfterLoop(this);
        visit_children(expr);
      }

      if (!boolean(expr->condition())) {
        auto a = gal::point_out_list(type_was_err(expr->condition()));

        diagnostics_->report_emplace(15, gal::into_list(std::move(a)));
      }

      update_return(expr, void_type(expr->loc()));
    }

    void visit(ast::ForExpression* expr) final {
      {
        auto _ = BeforeAfterLoop(this);
        visit_children(expr);
      }
    }

    void visit(ast::ReturnExpression* expr) final {
      visit_children(expr);

      if (auto value = expr->value_owner()) {
        auto& ret_val = ***value;

        if (expected_ == nullptr) {
          auto a = gal::point_out(*expr, gal::DiagnosticType::error, "return was here");

          diagnostics_->report_emplace(26, gal::into_list(std::move(a)));
        } else if (!try_make_compatible(*expected_, *value)) {
          auto a = type_was_err(ret_val);
          auto b = gal::point_out_part(*expected_,
              gal::DiagnosticType::note,
              absl::StrCat("expected type `", gal::to_string(*expected_), "` based on function signature"));

          diagnostics_->report_emplace(20, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
        }

        // we need to account for the possibility that there's an implicit conversion
        // in the `try_make_compatible`
        update_return(expr, (**value)->result().clone());
      } else {
        update_return(expr, void_type(expr->loc()));
      }
    }

    void visit(ast::BreakExpression* expr) final {
      visit_children(expr);

      if (!in_loop_) {
        auto a = gal::point_out(*expr, gal::DiagnosticType::error, "break was here");

        diagnostics_->report_emplace(26, gal::into_list(std::move(a)));
      }

      if (auto value = expr->value_mut()) {
        if (!can_break_with_value_) {
          auto a = gal::point_out_part(*expr, gal::DiagnosticType::error, "break was here");
          auto b = gal::point_out_part(**value, gal::DiagnosticType::note, "value being broken is here");

          diagnostics_->report_emplace(36, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
        }

        if (last_break_type_ != nullptr && !try_make_compatible(*last_break_type_, *expr->value_owner())) {
          auto a = gal::point_out_part(*last_break_type_,
              gal::DiagnosticType::note,
              absl::StrCat("last break was of type `", gal::to_string(*last_break_type_), "`"));
          auto b = type_was_err(**value);

          diagnostics_->report_emplace(37, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
        }

        // user will need to cast if they want something other than default with literals
        convert_intermediate(*expr->value_owner());

        // `value` may be invalided at this point
        auto type = (*expr->value())->result().clone();

        last_break_type_ = type.get();

        // while it won't actually *evaluate* to that, may as well
        // make it **possible** to use it like it does in something like `if-then`
        update_return(expr, std::move(type));
      } else {
        update_return(expr, void_type(expr->loc()));
      }
    }

    void visit(ast::ContinueExpression* expr) final {
      if (!in_loop_) {
        auto a = gal::point_out(*expr, gal::DiagnosticType::error, "continue was here");

        diagnostics_->report_emplace(26, gal::into_list(std::move(a)));
      }

      update_return(expr, void_type(expr->loc()));
    }

    void visit(ast::ImplicitConversionExpression*) final {
      // should not **ever** ever visit one of these, type-checker is what generates these,
      // and it's generated **after** the children of a node are visited.
      // unless nodes are getting visited multiple times (which is a bug), this should
      // never EVER be called
      assert(false);
    }

    void visit(ast::LoadExpression*) final {
      // see comment above
      assert(false);
    }

    void visit(ast::ReferenceType*) final {}

    void visit(ast::SliceType*) final {}

    void visit(ast::PointerType*) final {}

    void visit(ast::BuiltinIntegralType*) final {}

    void visit(ast::BuiltinFloatType*) final {}

    void visit(ast::BuiltinByteType*) final {}

    void visit(ast::BuiltinBoolType*) final {}

    void visit(ast::BuiltinCharType*) final {}

    void visit(ast::UserDefinedType*) final {}

    void visit(ast::FnPointerType*) final {}

    void visit(ast::DynInterfaceType*) final {}

    void visit(ast::VoidType*) final {}

    void visit(ast::NilPointerType*) final {}

    void visit(ast::ErrorType*) final {
      assert(false);
    }

    void visit(ast::IndirectionType*) final {
      assert(false);
    }

  private:
    class BeforeAfterLoop {
    public:
      explicit BeforeAfterLoop(TypeChecker* ptr, bool can_break_with_val = false) noexcept : ptr_{ptr} {
        ptr_->in_loop_ = true;
        ptr_->can_break_with_value_ = can_break_with_val;
        ptr_->last_break_type_ = nullptr;
      }

      ~BeforeAfterLoop() {
        ptr_->in_loop_ = false;
        ptr_->can_break_with_value_ = false;
        ptr_->last_break_type_ = nullptr;
      }

    private:
      TypeChecker* ptr_;
    };

#define GALLIUM_X_COMPATIBLE(name)                                                                                     \
  [[nodiscard]] static bool name(const ast::Expression& lhs, const ast::Type& rhs) noexcept {                          \
    return name(lhs.result(), rhs);                                                                                    \
  }                                                                                                                    \
  [[nodiscard]] static bool name(const ast::Type& lhs, const ast::Expression& rhs) noexcept {                          \
    return name(lhs, rhs.result());                                                                                    \
  }                                                                                                                    \
  [[nodiscard]] static bool name(const ast::Expression& lhs, const ast::Expression& rhs) noexcept {                    \
    return name(lhs.result(), rhs.result());                                                                           \
  }

    [[nodiscard]] std::uint64_t builtin_size_of_bits(const ast::Type& type) noexcept {
      switch (type.type()) {
        case TT::builtin_integral: {
          auto& builtin = gal::as<ast::BuiltinIntegralType>(type);

          if (auto width = ast::width_of(builtin.width())) {
            return static_cast<std::uint64_t>(*width) - (builtin.has_sign() ? 1 : 0);
          } else {
            return machine_.getPointerSizeInBits(layout_.getProgramAddressSpace()) - (builtin.has_sign() ? 1 : 0);
          }
        }
        case TT::builtin_char:
        case TT::builtin_bool:
        case TT::builtin_byte: {
          return 8;
        }
        case TT::builtin_float: {
          auto& builtin = gal::as<ast::BuiltinFloatType>(type);

          switch (builtin.width()) {
            case ast::FloatWidth::ieee_single: return 32;
            case ast::FloatWidth::ieee_double: return 64;
            case ast::FloatWidth::ieee_quadruple: return 128;
            default: assert(false); break;
          }

          break;
        }
        case TT::pointer:
        case TT::reference: {
          return machine_.getPointerSizeInBits(layout_.getProgramAddressSpace());
        }
        default: assert(false); break;
      }

      return 0;
    }

    void unwrap_indirection(std::unique_ptr<ast::Expression>* expr) noexcept {
      if ((*expr)->is(ET::group)) {
        auto* group = gal::as_mut<ast::GroupExpression>(expr->get());

        *expr = std::move(*group->expr_owner());

        unwrap_indirection(expr);
      } else if ((*expr)->is(ET::unary)) {
        auto* unary = gal::as_mut<ast::UnaryExpression>(expr->get());
        (void)gal::as<ast::IndirectionType>(unary->result());

        *expr = std::move(*unary->expr_owner());
      }
    }

    [[nodiscard]] static bool is_indirection_to(TT type, const ast::Expression& expr) noexcept {
      if (expr.result().is(TT::indirection)) {
        auto& indirection = gal::as<ast::IndirectionType>(expr.result());

        return indirection.produced().is(type);
      }

      return false;
    }

    [[nodiscard]] bool convertible(const ast::Type& into, const ast::Expression& expr) noexcept {
      // the "implicit conversion" isn't really a conversion, it's a load. the result is the same
      if (expr.result().is(TT::indirection)) {
        auto& indirection = gal::as<ast::IndirectionType>(expr.result());

        return indirection.produced() == into;
      }

      if (into.is(TT::slice) && expr.result().is(TT::reference)) {
        auto& ref = gal::as<ast::ReferenceType>(expr.result());

        if (ref.referenced().is(TT::array)) {
          auto& array = gal::as<ast::ArrayType>(ref.referenced());
          auto& slice = gal::as<ast::SliceType>(into);

          if (slice.mut()) {
            return mut(expr) && array.element_type() == slice.sliced();
          } else {
            return array.element_type() == slice.sliced();
          }
        }
      }

      // we can convert the magic literal type -> integral types
      // and the magic nil type -> pointer types
      return (expr.result().is(TT::unsized_integer) && into.is_one_of(TT::builtin_integral, TT::builtin_byte))
             || (expr.result().is(TT::nil_pointer) && into.is_one_of(TT::fn_pointer, TT::pointer));
    }

    bool implicit_convert(const ast::Type& expected,
        std::unique_ptr<ast::Expression>* self,
        std::unique_ptr<ast::Expression>* expr) noexcept {
      if (expected.is_one_of(TT::builtin_integral, TT::builtin_byte) && (*expr)->result().is(TT::unsized_integer)) {
        auto& literal = gal::as<ast::UnsizedIntegerType>((*expr)->result());

        if (gal::ipow(std::uint64_t{2}, builtin_size_of_bits(expected)) < literal.value()) {
          auto a = gal::point_out_part(expected, gal::DiagnosticType::note, "converting based on this");
          auto b = gal::point_out_part(**expr,
              gal::DiagnosticType::error,
              absl::StrCat("integer literal cannot fit in type `", gal::to_string(expected), "`"));

          auto c = gal::point_out_list(std::move(a), std::move(b));
          auto d = gal::single_message(absl::StrCat("note: max value for type `",
              gal::to_string(expected),
              "` is ",
              gal::ipow(std::uint64_t{2}, builtin_size_of_bits(expected)) - 1));

          diagnostics_->report_emplace(32, gal::into_list(std::move(c), std::move(d)));

          return false;
        }
      }

      if ((*expr)->result().is(TT::indirection)) {
        unwrap_indirection(expr);

        *self = std::make_unique<ast::LoadExpression>((*expr)->loc(), std::move(*expr));
      } else {
        *self = std::make_unique<ast::ImplicitConversionExpression>(std::move(*expr), expected.clone());
      }

      (*self)->result_update(expected.clone());

      return true;
    }

    [[nodiscard]] bool try_make_compatible(const ast::Type& expected, std::unique_ptr<ast::Expression>* expr) noexcept {
      if (identical(expected, **expr)) {
        return true;
      }

      if (convertible(expected, **expr)) {
        return implicit_convert(expected, expr, expr);
      }

      return false;
    }

    [[nodiscard]] static bool identical(const ast::Type& lhs, const ast::Type& rhs) noexcept {
      return (lhs == rhs);
    }

    GALLIUM_X_COMPATIBLE(identical)

    [[nodiscard]] static bool boolean(const ast::Type& type) noexcept {
      return type.is(TT::builtin_bool);
    }

    [[nodiscard]] static bool boolean(const ast::Expression& expr) noexcept {
      return boolean(expr.result());
    }

    [[nodiscard]] static bool integral(const ast::Type& type) noexcept {
      return type.is_one_of(TT::builtin_integral, TT::builtin_byte, TT::unsized_integer);
    }

    [[nodiscard]] static bool integral(const ast::Expression& expr) noexcept {
      return integral(expr.result());
    }

    [[nodiscard]] static bool arithmetic(const ast::Type& type) noexcept {
      return integral(type) || type.is(TT::builtin_float);
    }

    [[nodiscard]] static bool arithmetic(const ast::Expression& expr) noexcept {
      return arithmetic(expr.result());
    }

    template <typename T> bool check_mut(const ast::Expression& expr) noexcept {
      auto& entity = gal::as<T>(expr.result());

      return entity.mut();
    }

    [[nodiscard]] bool mut(const ast::Expression& expr) noexcept {
      // need to check this first: an id that maps to a `mut` view type
      // would break otherwise
      switch (expr.result().type()) {
        case TT::pointer: return check_mut<ast::PointerType>(expr);
        case TT::reference: return check_mut<ast::ReferenceType>(expr);
        case TT::indirection: return check_mut<ast::IndirectionType>(expr);
        case TT::slice: return check_mut<ast::SliceType>(expr);
        case TT::error: return true;
        default: break;
      }

      switch (expr.type()) {
        case ET::identifier: {
          // TODO: global mut variables?
          return false;
        }
        case ET::identifier_local: {
          auto& local = gal::as<ast::LocalIdentifierExpression>(expr);

          return (*resolver_.local(local.name()))->mut();
        }
        case ET::string_lit: {
          // always false, string literals are read-only
          return false;
        }
        default: break;
      }

      // default is false, temporaries and whatnot should not be able to be mutated for example
      return false;
    }

    GALLIUM_COLD gal::PointedOut report_not_mut(const ast::Expression& expr) {
      switch (expr.type()) {
        case ET::identifier: {
          auto& id = gal::as<ast::IdentifierExpression>(expr);

          return gal::point_out_part((*resolver_.constant(id.id()))->loc(),
              gal::DiagnosticType::note,
              "name referred to this, constants are never `mut`");
        }
        case ET::identifier_local: {
          auto& local = gal::as<ast::LocalIdentifierExpression>(expr);

          return gal::point_out_part((*resolver_.local(local.name()))->loc(),
              gal::DiagnosticType::note,
              "name referred to this, local binding is not `mut`");
        }
        default: break;
      }

      return type_was_note(expr);
    }

    [[nodiscard]] static bool lvalue(const ast::Expression& expr) noexcept {
      // identifiers all have some sort of address, field-access requires a struct object to exist,
      // array-access requires the array / slice to exist, string literals are magic
      return expr.is_one_of(ET::identifier, ET::identifier_local, ET::string_lit)
             || expr.result().is_one_of(TT::indirection, TT::error);
    }

    [[nodiscard]] static bool rvalue(const ast::Expression& expr) noexcept {
      return !lvalue(expr);
    }

    void update_return(ast::Expression* expr, std::unique_ptr<ast::Type> type) noexcept {
      expr->result_update(std::move(type));

      Expr::return_value(expr->result_mut());
    }

    [[nodiscard]] bool check_identical(ast::BinaryExpression* expr) {
      auto& lhs = expr->lhs();
      auto& rhs = expr->rhs();

      // we can always try to convert to make the expression valid by
      // sprinkling on some conversion magic
      if (!identical(lhs, rhs) && !try_make_compatible(lhs.result(), expr->rhs_owner())
          && !try_make_compatible(rhs.result(), expr->lhs_owner())) {
        auto a = gal::point_out_part(*expr, gal::DiagnosticType::error, "lhs and rhs were not of identical types");
        auto b = type_was_note(lhs, "left-hand side's ");
        auto c = type_was_note(rhs, "right-hand side's ");
        auto d = gal::point_out_list(std::move(a), std::move(b), std::move(c));

        diagnostics_->report_emplace(40, gal::into_list(std::move(d)));

        update_return(expr, error_type());

        return false;
      }

      // if one of them gets converted here, both of them will
      convert_intermediate(expr->lhs_owner());
      convert_intermediate(expr->rhs_owner());

      return true;
    }

    [[nodiscard]] bool check_condition(ast::BinaryExpression* expr,
        bool (*pred)(const ast::Expression&),
        std::int64_t code,
        std::string_view condition_name) {
      auto& lhs = expr->lhs();
      auto& rhs = expr->rhs();
      auto result = pred(lhs) && pred(rhs);

      if (!result) {
        auto a = gal::point_out_part(*expr,
            gal::DiagnosticType::error,
            absl::StrCat("lhs and rhs were not both ", condition_name));
        auto b = type_was_note(lhs, "left-hand side's ");
        auto c = type_was_note(rhs, "right-hand side's ");
        auto d = gal::point_out_list(std::move(a), std::move(b), std::move(c));
        auto e = gal::single_message(absl::StrCat("both left and right expressions for operator `",
            gal::binary_op_string(expr->op()),
            "` must be ",
            condition_name));

        diagnostics_->report_emplace(code, gal::into_list(std::move(d), std::move(e)));

        update_return(expr, error_type());
      }

      return result;
    }

    [[nodiscard]] bool check_binary_conditions(ast::BinaryExpression* expr,
        bool (*pred)(const ast::Expression&),
        std::int64_t code,
        std::string_view condition_name) noexcept {
      return check_condition(expr, pred, code, condition_name) && check_identical(expr);
    }

    [[nodiscard]] bool check_qualified_id(std::unique_ptr<ast::Expression>* expr,
        const ast::FullyQualifiedID& id) noexcept {
      if (!constant_only_) {
        if (auto overloads = resolver_.overloads(id)) {
          auto& overload_set = **overloads;
          auto fns = overload_set.fns();

          if (fns.size() == 1) {
            auto held = std::move(*expr);
            *expr = std::make_unique<ast::StaticGlobalExpression>(held->loc(), fns.front().decl_base());
            update_return(expr->get(), fn_pointer_for(fns.front().loc(), fns.front().proto()));

            return true;
          }

          if (ignore_ambiguous_fn_ref_) {
            auto held = std::move(*expr);
            *expr = std::make_unique<ast::IdentifierExpression>(held->loc(), id);
            update_return(expr->get(), std::make_unique<ast::ErrorType>());
            // intentionally not put a return type here, we're inside a call expr, and it will handle it

            return true;
          }

          auto a = gal::point_out(**expr, gal::DiagnosticType::error, "usage was here");
          auto b = gal::single_message(absl::StrCat("there were ", fns.size(), " potential overloads"));

          diagnostics_->report_emplace(19, gal::into_list(std::move(a), std::move(b)));

          update_return(expr->get(), error_type());
          return true;
        }
      }

      if (auto constant = resolver_.constant(id)) {
        auto& c = **constant;

        auto held = std::move(*expr);
        *expr = std::make_unique<ast::StaticGlobalExpression>(held->loc(), c);
        update_return(expr->get(), c.hint().clone());

        return true;
      }

      return false;
    }

    template <typename Arg, typename Fn = gal::Deref>
    void report_uncallable(const ast::SourceLoc& fn_loc,
        const ast::Expression& call_expr,
        absl::Span<const Arg> fn_args,
        absl::Span<std::unique_ptr<ast::Expression>> given_args,
        Fn mapper = {}) noexcept {
      auto fn_it = fn_args.begin();
      auto given_it = given_args.begin();

      for (; fn_it != fn_args.end() && given_it != given_args.end(); ++fn_it, ++given_it) {
        auto& type = mapper(*fn_it);
        auto& expr = **given_it;

        if (!try_make_compatible(type, &*given_it)) {
          auto a = type_was_err(expr);
          auto b = gal::point_out_part(type,
              gal::DiagnosticType::note,
              absl::StrCat("expected type `", gal::to_string(type), "` based on this"));

          diagnostics_->report_emplace(23, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
        }
      }

      if (fn_it != fn_args.end() || given_it != given_args.end()) {
        auto vec = std::vector<gal::PointedOut>{};
        vec.push_back(gal::point_out_part(fn_loc, gal::DiagnosticType::note, "function signature is here"));
        vec.push_back(gal::point_out_part(call_expr,
            gal::DiagnosticType::error,
            absl::StrCat("expected ",
                fn_args.size(),
                gal::make_plural(fn_args.size(), " arguments"),
                " but got ",
                given_args.size())));

        // 25 = too few, 24 = too many
        auto error_code = (fn_it != fn_args.end()) ? 25 : 24;

        diagnostics_->report_emplace(error_code, gal::into_list(gal::point_out_list(std::move(vec))));
      }
    }

    template <typename Arg, typename Fn = gal::Deref>
    [[nodiscard]] bool callable(absl::Span<const Arg> fn_args,
        absl::Span<std::unique_ptr<ast::Expression>> given_args,
        Fn mapper = {}) noexcept {
      auto fn_it = fn_args.begin();
      auto given_it = given_args.begin();
      auto had_failure = false;

      for (; fn_it != fn_args.end() && given_it != given_args.end(); ++fn_it, ++given_it) {
        if (!try_make_compatible(mapper(*fn_it), &*given_it)) {
          had_failure = true;
        }
      }

      if (fn_it != fn_args.end() || given_it != given_args.end()) {
        had_failure = true;
      }

      return !had_failure;
    }

    GALLIUM_COLD void report_ambiguous(const ast::Expression& expr,
        const gal::OverloadSet& set,
        absl::Span<std::unique_ptr<ast::Expression>> args) noexcept {
      auto vec = std::vector<gal::PointedOut>{};
      auto mapper = [](const gal::ast::Argument& arg) -> decltype(auto) {
        return arg.type();
      };

      for (auto& overload : set.fns()) {
        if (callable(overload.proto().args(), args, mapper)) {
          vec.push_back(gal::point_out_part(overload.decl_base(), gal::DiagnosticType::note, "candidate is here"));
        }
      }

      vec.push_back(gal::point_out_part(expr, gal::DiagnosticType::error, "ambiguous call was here"));
      diagnostics_->report_emplace(27, gal::into_list(gal::point_out_list(std::move(vec))));
    }

    GALLIUM_COLD void report_no_overload(const ast::Expression& expr,
        const gal::OverloadSet& set,
        absl::Span<std::unique_ptr<ast::Expression>> args) noexcept {
      auto vec = std::vector<gal::PointedOut>{};

      for (auto& overload : set.fns()) {
        vec.push_back(gal::point_out_part(overload.decl_base(), gal::DiagnosticType::note, "candidate is here"));
      }

      vec.push_back(gal::point_out_part(expr, gal::DiagnosticType::error, "no matching overload for this call"));
      auto a = gal::single_message(absl::StrCat("arguments were of type (",
          absl::StrJoin(args,
              "",
              [](std::string* out, const std::unique_ptr<ast::Expression>& arg) {
                absl::StrAppend(out, gal::to_string(arg->result()));
              }),
          ")"));
      diagnostics_->report_emplace(51, gal::into_list(gal::point_out_list(std::move(vec)), std::move(a)));
    }

    std::optional<const gal::Overload*> select_overload(const ast::Expression& expr,
        const gal::OverloadSet& set,
        absl::Span<std::unique_ptr<ast::Expression>> args) noexcept {
      const auto* ptr = static_cast<const gal::Overload*>(nullptr);
      auto mapper = [](const gal::ast::Argument& arg) -> decltype(auto) {
        return arg.type();
      };

      for (auto& overload : set.fns()) {
        if (callable(overload.proto().args(), args, mapper)) {
          if (ptr != nullptr) {
            report_ambiguous(expr, set, args);

            return ptr;
          }

          ptr = &overload;
        }
      }

      if (ptr == nullptr) {
        report_no_overload(expr, set, args);
      }

      return (ptr != nullptr) ? std::make_optional(ptr) : std::nullopt;
    }

    GALLIUM_COLD void cast_error(ast::CastExpression* expr, std::string_view msg, std::string_view help = "") noexcept {
      auto a = gal::point_out(*expr, gal::DiagnosticType::error);
      auto b = gal::single_message(std::string{msg});
      auto vec = gal::into_list(std::move(a), std::move(b));

      if (!help.empty()) {
        vec.push_back(gal::single_message(std::string{help}));
      }

      diagnostics_->report_emplace(17, std::move(vec));

      return update_return(expr, error_type());
    }

    void convert_intermediate(std::unique_ptr<ast::Expression>* ptr) noexcept {
      if ((*ptr)->result().is(TT::indirection)) {
        auto& indirection = gal::as<ast::IndirectionType>((*ptr)->result());

        implicit_convert(*indirection.produced().clone(), ptr, ptr);
      } else if ((*ptr)->result().is(TT::nil_pointer)) {
        implicit_convert(byte_ptr, ptr, ptr);
      } else if ((*ptr)->result().is(TT::unsized_integer)) {
        implicit_convert(default_int, ptr, ptr);
      }
    }

    [[nodiscard]] static ast::Type* array_type(ast::Type* type) noexcept {
      switch (type->type()) {
        case TT::reference: {
          auto* array = gal::as_mut<ast::ReferenceType>(type);

          return array_type(array->referenced_mut());
        }
        case TT::pointer: {
          auto* ptr = gal::as_mut<ast::PointerType>(type);

          return array_type(ptr->pointed_mut());
        }
        case TT::slice: {
          auto* slice = gal::as_mut<ast::SliceType>(type);

          return slice->sliced_mut();
        }
        case TT::array: {
          auto* slice = gal::as_mut<ast::ArrayType>(type);

          return slice->element_type_mut();
        }
        default: assert(false); return nullptr;
      }
    }

    std::unique_ptr<ast::Expression> dereference(std::unique_ptr<ast::Expression> expr) noexcept {
      auto& loc = expr->loc();

      auto& reference = gal::as<ast::ReferenceType>(expr->result());
      auto is_mut = mut(*expr);

      auto deref = std::make_unique<ast::UnaryExpression>(loc, ast::UnaryOp::dereference, std::move(expr));
      deref->result_update(std::make_unique<ast::IndirectionType>(loc, reference.referenced().clone(), is_mut));

      return deref;
    }

    void auto_deref(std::unique_ptr<ast::Expression>* expr) noexcept {
      if ((*expr)->result().is(TT::reference)) {
        auto held = std::move(*expr);

        *expr = dereference(std::move(held));
      }
    }

    ast::Type* expected_ = nullptr;
    ast::Type* last_break_type_ = nullptr;
    ast::Program* program_;
    gal::DiagnosticReporter* diagnostics_;
    gal::NameResolver resolver_;
    const llvm::TargetMachine& machine_;
    const llvm::DataLayout layout_;
    bool constant_only_ = false;
    bool in_loop_ = false;
    bool can_break_with_value_ = false;
    bool ignore_ambiguous_fn_ref_ = false;
  }; // namespace
} // namespace

bool gal::type_check(ast::Program* program,
    const llvm::TargetMachine& machine,
    gal::DiagnosticReporter* reporter) noexcept {
  return TypeChecker(program, machine, reporter).type_check();
}
