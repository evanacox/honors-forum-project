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
#include <stack>
#include <vector>

namespace ast = gal::ast;
namespace internal = gal::internal;

namespace {
  using DT = ast::DeclType;
  using ET = ast::ExprType;
  using TT = ast::TypeType;
  using ST = ast::StmtType;

  std::unique_ptr<ast::Type> int_type(ast::SourceLoc loc, int width) noexcept {
    return std::make_unique<ast::BuiltinIntegralType>(std::move(loc), true, static_cast<ast::IntegerWidth>(width));
  }

  std::unique_ptr<ast::Type> uint_type(ast::SourceLoc loc, int width) noexcept {
    return std::make_unique<ast::BuiltinIntegralType>(std::move(loc), false, static_cast<ast::IntegerWidth>(width));
  }

  std::unique_ptr<ast::Type> slice_of(ast::SourceLoc loc, std::unique_ptr<ast::Type> type) noexcept {
    return std::make_unique<ast::SliceType>(std::move(loc), std::move(type));
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

  gal::PointedOut type_was(const ast::Expression& expr, gal::DiagnosticType type) noexcept {
    auto msg = absl::StrCat("real type was `", gal::to_string(expr.result()), "`");

    return gal::point_out_part(expr, type, std::move(msg));
  }

  gal::PointedOut type_was_err(const ast::Expression& expr) noexcept {
    return type_was(expr, gal::DiagnosticType::error);
  }

  gal::PointedOut type_was_note(const ast::Expression& expr) noexcept {
    return type_was(expr, gal::DiagnosticType::note);
  }

  gal::PointedOut expected_type(const ast::Type& type) noexcept {
    auto msg = absl::StrCat("expected type `", gal::to_string(type), "`");

    return gal::point_out_part(type, gal::DiagnosticType::note, std::move(msg));
  }

  class TypeChecker final : public ast::AnyVisitorBase<void, ast::Type*> {
    using Expr = ast::ExpressionVisitor<ast::Type*>;
    using Base = ast::AnyVisitorBase<void, ast::Type*>;

  public:
    explicit TypeChecker(ast::Program* program, gal::DiagnosticReporter* reporter) noexcept
        : program_{program},
          diagnostics_{reporter},
          resolver_{program_, diagnostics_} {}

    bool type_check() noexcept {
      if (diagnostics_->had_error()) {
        return false;
      }

      Base::walk_ast(program_);

      return !diagnostics_->had_error();
    }

    void visit(ast::ImportDeclaration*) final {}

    void visit(ast::ImportFromDeclaration*) final {}

    void visit(ast::FnDeclaration* declaration) final {
      expected_ = declaration->proto_mut()->return_type_mut();

      declaration->body_mut()->accept(this);
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
        if (!type_compatible(**hint, expr_type)) {
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
      update_return(expr, slice_of(expr->loc(), uint_type(expr->loc(), 8)));
    }

    void visit(ast::IntegerLiteralExpression* expr) final {
      update_return(expr, int_type(expr->loc(), 64));
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
        if (auto type = resolver_.local(expr->name())) {
          return update_return(expr, (*type)->clone());
        }
      }

      if (auto qualified = resolver_.qualified_for(ast::UnqualifiedID{std::nullopt, std::string{expr->name()}})) {
        auto& [id, _] = *qualified;

        // why can't we use structured-binding variables inside of lambdas? aaaaaaaaa
        auto* id_p = &id;
        auto result = check_qualified_id(expr, id, [&, this](std::unique_ptr<ast::Type> type) {
          replace_self(std::make_unique<ast::IdentifierExpression>(expr->loc(), std::move(*id_p)));
          update_return(self_expr(), std::move(type));
        });

        if (result) {
          return;
        }
      }

      auto a = gal::point_out(*expr, gal::DiagnosticType::error, "usage was here");

      diagnostics_->report_emplace(18, gal::into_list(std::move(a)));

      update_return(expr, error_type());
    }

    void visit(ast::IdentifierExpression* expr) final {
      auto id = expr->id();

      auto result = check_qualified_id(expr, id, [&, this](std::unique_ptr<ast::Type> type) {
        update_return(expr, std::move(type));
      });

      if (!result) {
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

      auto& type = internal::debug_cast<const ast::UserDefinedType&>(expr->struct_type());

      if (auto decl = resolver_.struct_type(type.id())) {
        for (auto& s_field : (*decl)->fields()) {
          auto init_fields = expr->fields();
          auto init_field = absl::c_find_if(init_fields, [&](const ast::FieldInitializer& field) {
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

          if (!type_compatible(s_field.type(), init_field->init())) {
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

    void visit(ast::CallExpression* expr) final {
      for (auto& arg : expr->args_mut()) {
        Base::accept(&arg);
      }

      auto args = expr->args();

      // if it's not an identifier, it could be an identifier-local that gets translated to an identifier
      if (!expr->callee().is(ET::identifier)) {
        Base::accept(expr->callee_owner());
      }

      // we need to do special handling here in order to not break over function overloading n whatnot
      if (expr->callee().is(ET::identifier)) {
        auto& identifier = internal::debug_cast<const ast::IdentifierExpression&>(expr->callee());

        if (auto overloads = resolver_.overloads(identifier.id())) {
          if (auto overload = select_overload(*expr, **overloads, args)) {
            replace_self(ast::StaticCallExpression::from_call(identifier.id(), **overload, expr));

            return update_return(self_expr(), fn_pointer_for(expr->loc(), (*overload)->proto()));
          } else {
            return update_return(expr, error_type());
          }
        }

        auto a = gal::point_out(*expr, gal::DiagnosticType::error, "usage was here");
        diagnostics_->report_emplace(28, gal::into_list(std::move(a)));

        return update_return(expr, error_type());
      }

      if (expr->callee().result().is(TT::fn_pointer)) {
        auto& callee = expr->callee();
        auto& fn_ptr_type = internal::debug_cast<const ast::FnPointerType&>(callee.result());

        if (callable(callee.loc(), callee, fn_ptr_type.args(), args)) {
          return update_return(expr, fn_ptr_type.return_type().clone());
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

    void visit(ast::IndexExpression*) final {}

    void visit(ast::FieldAccessExpression*) final {}

    void visit(ast::GroupExpression* expr) final {
      visit_children(expr);

      update_return(expr, expr->expr().result().clone());
    }

    void visit(ast::UnaryExpression*) final {}

    void visit(ast::BinaryExpression*) final {
      // TODO: model lvalues vs. rvalues
      // TODO: remember: IMMUTABILITY AND := !!!!
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

    void visit(ast::CastExpression* expr) final {
      visit_children(expr);

      static ast::PointerType mut_byte_ptr{ast::SourceLoc::nonexistent(),
          true,
          byte_type(ast::SourceLoc::nonexistent())};

      static ast::PointerType byte_ptr{ast::SourceLoc::nonexistent(), false, byte_type(ast::SourceLoc::nonexistent())};

      if (!expr->unsafe()) {
        auto& result = expr->castee().result();

        switch (expr->cast_to().type()) {
          case TT::builtin_integral: [[fallthrough]];
          case TT::builtin_float: [[fallthrough]];
          case TT::builtin_bool: [[fallthrough]];
          case TT::builtin_byte: [[fallthrough]];
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
          case TT::dyn_interface: [[fallthrough]];
          case TT::user_defined: {
            return cast_error(expr, "cannot cast between user-defined types");
          }
          case TT::fn_pointer: {
            if (result.is(TT::nil_pointer) || type_compatible(result, mut_byte_ptr)
                || type_compatible(result, byte_ptr)) {
              return update_return(expr, expr->cast_to().clone());
            }

            return cast_error(expr,
                "cannot cast any type besides a `byte` pointer to a fn pointer",
                "help: try casting to `*const byte` first");
          }
          case TT::nil_pointer: [[fallthrough]];
          case TT::user_defined_unqualified: [[fallthrough]];
          case TT::dyn_interface_unqualified: [[fallthrough]];
          case TT::error: [[fallthrough]];
          case TT::reference: [[fallthrough]];
          case TT::slice: [[fallthrough]];
          case TT::pointer: [[fallthrough]];
          default: assert(false);
        }
      }
    }

    void visit(ast::IfThenExpression* expr) final {
      visit_children(expr);

      if (!bool_compatible(expr->condition())) {
        auto a = gal::point_out_list(type_was_err(expr->condition()));

        diagnostics_->report_emplace(15, gal::into_list(std::move(a)));
      }

      // compatible != identical, say its `if thing then nil else &a`. no accidental
      // and severely underpowered type inference is getting in until this compiler is ready!
      if (!type_identical(expr->true_branch(), expr->false_branch())) {
        auto a = gal::point_out_part(*expr, gal::DiagnosticType::error);
        auto b = type_was_note(expr->true_branch());
        auto c = type_was_note(expr->false_branch());

        diagnostics_->report_emplace(16, gal::into_list(gal::point_out_list(std::move(a), std::move(b), std::move(c))));

        update_return(expr, error_type());
      } else {
        update_return(expr, expr->true_branch().result().clone());
      }
    }

    void visit(ast::IfElseExpression*) final {
      // todo
    }

    void visit(ast::BlockExpression* expr) final {
      resolver_.enter_scope();

      visit_children(expr);

      if (auto stmts = expr->statements(); !stmts.empty() && expr->statements().back()->is(ST::expr)) {
        auto& expr_stmt = gal::internal::debug_cast<const ast::ExpressionStatement&>(*stmts.back());

        update_return(expr, expr_stmt.expr().result().clone());
      } else {
        update_return(expr, void_type(expr->loc()));
      }

      resolver_.leave_scope();
    }

    void visit(ast::LoopExpression* expr) final {
      in_loop_ = true;
      visit_children(expr);
      in_loop_ = false;

      update_return(expr, void_type(expr->loc()));
    }

    void visit(ast::WhileExpression* expr) final {
      in_loop_ = true;
      visit_children(expr);
      in_loop_ = false;

      if (!bool_compatible(expr->condition())) {
        auto a = gal::point_out_list(type_was_err(expr->condition()));

        diagnostics_->report_emplace(15, gal::into_list(std::move(a)));
      }

      update_return(expr, void_type(expr->loc()));
    }

    void visit(ast::ForExpression* expr) final {
      in_loop_ = true;
      visit_children(expr);
      in_loop_ = false;
    }

    void visit(ast::ReturnExpression* expr) final {
      visit_children(expr);

      if (auto value = expr->value()) {
        auto& ret_val = **value;

        if (expected_ == nullptr) {
          auto a = gal::point_out(*expr, gal::DiagnosticType::error, "return was here");

          diagnostics_->report_emplace(25, gal::into_list(std::move(a)));
        }

        if (expected_ != nullptr && !type_compatible(*expected_, ret_val)) {
          auto a = type_was_err(ret_val);
          auto b = gal::point_out_part(*expected_,
              gal::DiagnosticType::note,
              absl::StrCat("expected type `", gal::to_string(*expected_), "` based on function signature"));

          diagnostics_->report_emplace(20, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
        }

        // while it won't actually *evaluate* to that, may as well
        // make it **possible** to use it like it does in something like `if-then`
        update_return(expr, ret_val.result().clone());
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

      if (auto value = expr->value()) {
        // while it won't actually *evaluate* to that, may as well
        // make it **possible** to use it like it does in something like `if-then`
        update_return(expr, (*value)->result().clone());
      } else {
        update_return(expr, void_type(expr->loc()));
      }
    }

    void visit(ast::ContinueExpression* expr) final {
      if (!in_loop_) {
        auto a = gal::point_out(*expr, gal::DiagnosticType::error, "break was here");

        diagnostics_->report_emplace(26, gal::into_list(std::move(a)));
      }

      update_return(expr, void_type(expr->loc()));
    }

    void visit(ast::ImplicitConversionExpression*) final {
      // shouldnt ever visit, typechecker is what generates these
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

    [[nodiscard]] static bool type_compatible(const ast::Type& lhs, const ast::Type& rhs) noexcept {
      if (lhs.is_one_of(TT::pointer, TT::fn_pointer) && rhs.is(TT::nil_pointer)) {
        return true;
      }

      if (rhs.is_one_of(TT::pointer, TT::fn_pointer) && lhs.is(TT::nil_pointer)) {
        return true;
      }

      return lhs == rhs;
    }

    GALLIUM_X_COMPATIBLE(type_compatible)

    [[nodiscard]] static bool type_identical(const ast::Type& lhs, const ast::Type& rhs) noexcept {
      return lhs == rhs;
    }

    GALLIUM_X_COMPATIBLE(type_identical)

    [[nodiscard]] static bool bool_compatible(const ast::Type& type) noexcept {
      static ast::BuiltinBoolType single_bool{ast::SourceLoc::nonexistent()};

      return type == single_bool;
    }

    [[nodiscard]] static bool bool_compatible(const ast::Expression& expr) noexcept {
      return bool_compatible(expr.result());
    }

    void update_return(ast::Expression* expr, std::unique_ptr<ast::Type> type) noexcept {
      expr->result_update(std::move(type));

      Expr::return_value(expr->result_mut());
    }

    template <typename Fn>
    bool check_qualified_id(ast::Expression* expr, const ast::FullyQualifiedID& id, Fn f) noexcept {
      if (!constant_only_) {
        if (auto overloads = resolver_.overloads(id)) {
          auto& overload_set = **overloads;
          auto fns = overload_set.fns();

          if (fns.size() == 1) {
            f(fn_pointer_for(fns.front().loc(), fns.front().proto()));
            return true;
          }

          auto a = gal::point_out(*expr, gal::DiagnosticType::error, "usage was here");
          auto b = gal::single_message(absl::StrCat("there were ", fns.size(), " potential overloads"));

          diagnostics_->report_emplace(19, gal::into_list(std::move(a), std::move(b)));

          update_return(expr, error_type());
          return true;
        }
      }

      if (auto constant = resolver_.constant(id)) {
        auto& c = **constant;

        f(c.hint().clone());
        return true;
      }

      return false;
    }

    template <typename Arg, typename Fn = gal::Deref>
    bool callable(const ast::SourceLoc& fn_loc,
        const ast::Expression& call_expr,
        absl::Span<const Arg> fn_args,
        absl::Span<const std::unique_ptr<ast::Expression>> given_args,
        Fn mapper = {}) noexcept {
      auto fn_it = fn_args.begin();
      auto given_it = given_args.begin();
      auto had_failure = false;

      for (; fn_it != fn_args.end() && given_it != given_args.end(); ++fn_it, ++given_it) {
        auto& type = mapper(*fn_it);
        auto& expr = **given_it;

        if (!type_compatible(type, expr)) {
          auto a = type_was_err(expr);
          auto b = gal::point_out_part(type,
              gal::DiagnosticType::note,
              absl::StrCat("expected type `", gal::to_string(type), "` based on this"));

          diagnostics_->report_emplace(23, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));

          had_failure = true;
        }
      }

      if (fn_it != fn_args.end() || given_it != given_args.end()) {
        had_failure = true;

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

      return !had_failure;
    }

    GALLIUM_COLD void report_ambiguous(const ast::Expression& expr,
        const gal::OverloadSet& set,
        absl::Span<const std::unique_ptr<ast::Expression>> args) noexcept {
      auto vec = std::vector<gal::PointedOut>{};
      auto mapper = [](const gal::ast::Argument& arg) -> decltype(auto) {
        return arg.type();
      };

      for (auto& overload : set.fns()) {
        if (callable(overload.loc(), expr, overload.proto().args(), args, mapper)) {
          vec.push_back(gal::point_out_part(overload.decl_base(), gal::DiagnosticType::note, "candidate is here"));
        }
      }

      vec.push_back(gal::point_out_part(expr, gal::DiagnosticType::error, "ambiguous call was here"));
      diagnostics_->report_emplace(27, gal::into_list(gal::point_out_list(std::move(vec))));
    }

    std::optional<const gal::Overload*> select_overload(const ast::Expression& expr,
        const gal::OverloadSet& set,
        absl::Span<const std::unique_ptr<ast::Expression>> args) noexcept {
      const auto* ptr = static_cast<const gal::Overload*>(nullptr);
      auto mapper = [](const gal::ast::Argument& arg) -> decltype(auto) {
        return arg.type();
      };

      for (auto& overload : set.fns()) {
        if (callable(overload.loc(), expr, overload.proto().args(), args, mapper)) {
          if (ptr != nullptr) {
            report_ambiguous(expr, set, args);

            return ptr;
          }

          ptr = &overload;
        }
      }

      return (ptr != nullptr) ? std::make_optional(ptr) : std::nullopt;
    }

    ast::Type* expected_ = nullptr;
    ast::Program* program_;
    gal::DiagnosticReporter* diagnostics_;
    gal::NameResolver resolver_;
    bool constant_only_ = false;
    bool in_loop_ = false;
  }; // namespace
} // namespace

bool gal::type_check(ast::Program* program, gal::DiagnosticReporter* reporter) noexcept {
  return TypeChecker(program, reporter).type_check();
}
