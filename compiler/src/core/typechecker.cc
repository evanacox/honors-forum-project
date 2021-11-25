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
#include "../utility/pretty.h"
#include "./environment.h"
#include "./error_reporting.h"
#include "./name_resolver.h"
#include "absl/container/flat_hash_map.h"
#include <stack>
#include <vector>

namespace ast = gal::ast;

namespace {
  ast::VoidType void_type{ast::SourceLoc::nonexistent()};

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

  class TypeChecker final : public ast::DeclarationVisitor<void>,
                            public ast::StatementVisitor<ast::Type*>,
                            public ast::ExpressionVisitor<ast::Type*>,
                            public ast::TypeVisitor<void> {
    using Stmt = ast::StatementVisitor<ast::Type*>;
    using Expr = ast::ExpressionVisitor<ast::Type*>;

  public:
    explicit TypeChecker(ast::Program* program) noexcept : resolver_{program, &diagnostics_} {
      for (auto& decl : program->decls_mut()) {
        decl->accept(this);
      }
    }

    std::vector<gal::Diagnostic> take_diagnostics() noexcept {
      return std::move(diagnostics_);
    }

    void visit(ast::ImportDeclaration*) final {}

    void visit(ast::ImportFromDeclaration*) final {}

    void visit(ast::FnDeclaration* declaration) final {
      expected_ = declaration->proto_mut()->return_type_mut();

      declaration->body_mut()->accept(this);
    }

    void visit(ast::StructDeclaration*) final {}

    void visit(ast::ClassDeclaration*) final {}

    void visit(ast::TypeDeclaration*) final {}

    void visit(ast::MethodDeclaration*) final {}

    void visit(ast::ExternalFnDeclaration*) final {}

    void visit(ast::ExternalDeclaration*) final {}

    void visit(ast::ConstantDeclaration*) final {}

    void visit(ast::BindingStatement* statement) final {
      auto expr_type = statement->initializer_mut()->accept(this);

      if (auto hint = statement->hint(); **hint != *expr_type) {
        auto error_point_out = gal::point_out(statement->initializer(),
            gal::DiagnosticType::error,
            absl::StrCat("real type was `", gal::to_string(**hint), "`"));

        auto hint_point_out = gal::point_out(**hint,
            gal::DiagnosticType::note,
            absl::StrCat("expected type `", gal::to_string(**hint), "`"));

        auto vec = gal::into_list(std::move(error_point_out), std::move(hint_point_out));

        diagnostics_.emplace_back(7, std::move(vec));
      }

      Stmt::return_value(&void_type);
    }

    void visit(ast::ExpressionStatement* stmt) final {
      Stmt::return_value(stmt->expr_mut()->accept(this));
    }

    void visit(ast::AssertStatement* stmt) final {
      stmt->assertion_mut()->accept(this);
      stmt->message_mut()->accept(this);

      Stmt::return_value(&void_type);
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

    void visit(ast::UnqualifiedIdentifierExpression*) final {}

    void visit(ast::IdentifierExpression*) final {}

    void visit(ast::StructExpression* expr) final {
      if (!expr->struct_type().is_one_of(ast::TypeType::user_defined, ast::TypeType::user_defined_unqualified)) {
        auto error = gal::point_out(*expr, gal::DiagnosticType::error);

        diagnostics_.emplace_back(10, gal::into_list(std::move(error)));

        return update_return(expr, std::make_unique<ast::ErrorType>());
      }

      return update_return(expr, expr->struct_type().clone());
    }

    void visit(ast::CallExpression*) final {}

    void visit(ast::MethodCallExpression*) final {}

    void visit(ast::StaticMethodCallExpression*) final {}

    void visit(ast::IndexExpression*) final {}

    void visit(ast::FieldAccessExpression*) final {}

    void visit(ast::GroupExpression*) final {}

    void visit(ast::UnaryExpression*) final {}

    void visit(ast::BinaryExpression*) final {}

    void visit(ast::CastExpression*) final {}

    void visit(ast::IfThenExpression*) final {}

    void visit(ast::IfElseExpression*) final {}

    void visit(ast::BlockExpression* expr) final {
      auto* last_type = static_cast<ast::Type*>(&void_type);

      for (auto& stmt : expr->statements_mut()) {
        last_type = stmt->accept(this);
      }

      expr->result_update(last_type->clone());
    }

    void visit(ast::LoopExpression*) final {}

    void visit(ast::WhileExpression*) final {}

    void visit(ast::ForExpression*) final {}

    void visit(ast::ReturnExpression*) final {}

    void visit(ast::BreakExpression*) final {}

    void visit(ast::ContinueExpression*) final {}

    void visit(ast::ReferenceType*) final {}

    void visit(ast::SliceType*) final {}

    void visit(ast::PointerType*) final {}

    void visit(ast::BuiltinIntegralType*) final {}

    void visit(ast::BuiltinFloatType*) final {}

    void visit(ast::BuiltinByteType*) final {}

    void visit(ast::BuiltinBoolType*) final {}

    void visit(ast::BuiltinCharType*) final {}

    void visit(ast::UnqualifiedUserDefinedType*) final {}

    void visit(ast::UserDefinedType*) final {}

    void visit(ast::FnPointerType*) final {}

    void visit(ast::UnqualifiedDynInterfaceType*) final {}

    void visit(ast::DynInterfaceType*) final {}

    void visit(ast::VoidType*) final {}

    void visit(ast::NilPointerType*) final {}

  private:
    [[nodiscard]] bool matches(ast::Type* type) noexcept {
      return (expected_ == nullptr) || (*expected_ == *type);
    }

    void update_return(ast::Expression* expr, std::unique_ptr<ast::Type> type) noexcept {
      expr->result_update(std::move(type));

      Expr::return_value(*expr->result_mut());
    }

    ast::Type* expected_ = nullptr;
    std::vector<gal::Diagnostic> diagnostics_;
    gal::NameResolver resolver_;
  };
} // namespace

std::optional<std::vector<gal::Diagnostic>> gal::type_check(ast::Program* program) noexcept {
  auto diagnostics = TypeChecker(program).take_diagnostics();

  if (diagnostics.empty()) {
    return std::nullopt;
  } else {
    return diagnostics;
  }
}
