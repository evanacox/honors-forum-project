//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#pragma once

#include "./nodes/declaration.h"
#include "./nodes/expression.h"
#include "./nodes/statement.h"
#include "./nodes/type.h"
#include "./visitors/declaration_visitor.h"
#include "./visitors/expression_visitor.h"
#include "./visitors/statement_visitor.h"
#include "./visitors/type_visitor.h"

namespace gal::ast {
  template <typename T>
  class AnyVisitor : public DeclarationVisitor<T>,
                     public ExpressionVisitor<T>,
                     public StatementVisitor<T>,
                     public TypeVisitor<T> {};

  template <typename T> class AnyVisitorBase : public AnyVisitor<T> {
  public:
    void visit(ReferenceType* type) override {
      accept(type->referenced_owner());
    }

    void visit(SliceType* type) override {
      accept(type->sliced_owner());
    }

    void visit(PointerType* type) override {
      accept(type->pointed_owner());
    }

    void visit(BuiltinIntegralType*) override {}

    void visit(BuiltinFloatType*) override {}

    void visit(BuiltinByteType*) override {}

    void visit(BuiltinBoolType*) override {}

    void visit(BuiltinCharType*) override {}

    void visit(UnqualifiedUserDefinedType*) override {}

    void visit(UserDefinedType*) override {}

    void visit(FnPointerType* type) override {
      for (auto& arg : type->args_mut()) {
        accept(&arg);
      }

      accept(type->return_type_owner());
    }

    void visit(UnqualifiedDynInterfaceType*) override {}

    void visit(DynInterfaceType*) override {}

    void visit(VoidType*) override {}

    void visit(NilPointerType*) override {}

    void visit(ImportDeclaration*) override {}

    void visit(ImportFromDeclaration*) override {}

    void visit(FnDeclaration* declaration) override {
      accept(declaration->body_owner());
    }

    void visit(StructDeclaration* declaration) override {
      for (auto& field : declaration->fields_mut()) {
        accept(field.type_owner());
      }
    }

    void visit(ClassDeclaration*) override {}

    void visit(TypeDeclaration* declaration) override {
      accept(declaration->aliased_owner());
    }

    void visit(MethodDeclaration*) override {}

    void visit(ExternalFnDeclaration* declaration) override {
      accept_proto(declaration->proto_mut());
    }

    void visit(ExternalDeclaration* declaration) override {
      for (auto& fn : declaration->externals_mut()) {
        accept(&fn);
      }
    }

    void visit(ConstantDeclaration* declaration) override {
      accept(declaration->hint_owner());
      accept(declaration->initializer_owner());
    }

    void visit(StringLiteralExpression*) override {}

    void visit(IntegerLiteralExpression*) override {}

    void visit(FloatLiteralExpression*) override {}

    void visit(BoolLiteralExpression*) override {}

    void visit(CharLiteralExpression*) override {}

    void visit(NilLiteralExpression*) override {}

    void visit(UnqualifiedIdentifierExpression*) override {}

    void visit(IdentifierExpression*) override {}

    void visit(StructExpression* expression) override {
      accept(expression->struct_type_owner());

      for (auto& field : expression->fields_mut()) {
        accept(field.init_owner());
      }
    }

    void visit(CallExpression* expression) override {
      accept(expression->callee_owner());

      for (auto& arg : expression->args_mut()) {
        accept(&arg);
      }
    }

    void visit(MethodCallExpression*) override {}

    void visit(StaticMethodCallExpression*) override {}

    void visit(IndexExpression* expression) override {
      accept(expression->callee_owner());

      for (auto& arg : expression->args_mut()) {
        accept(&arg);
      }
    }

    void visit(FieldAccessExpression* expression) override {
      accept(expression->object_owner());
    }

    void visit(GroupExpression* expression) override {
      accept(expression->expr_owner());
    }

    void visit(UnaryExpression* expression) override {
      accept(expression->expr_owner());
    }

    void visit(BinaryExpression* expression) override {
      accept(expression->lhs_owner());
      accept(expression->rhs_owner());
    }

    void visit(CastExpression* expression) override {
      accept(expression->castee_owner());
      accept(expression->cast_to_owner());
    }

    void visit(IfThenExpression* expression) override {
      accept(expression->condition_owner());
      accept(expression->true_branch_owner());
      accept(expression->false_branch_owner());
    }

    void visit(IfElseExpression* expression) override {
      accept(expression->condition_owner());
      accept(expression->block_owner());

      for (auto& elif : expression->elif_blocks_mut()) {
        accept(elif.condition_owner());
        accept(elif.block_owner());
      }

      if (auto else_block = expression->else_block_owner()) {
        accept(*else_block);
      }
    }

    void visit(BlockExpression* expression) override {
      for (auto& stmt : expression->statements_mut()) {
        accept(&stmt);
      }
    }

    void visit(LoopExpression* expression) override {
      accept(expression->body_owner());
    }

    void visit(WhileExpression* expression) override {
      accept(expression->condition_owner());
      accept(expression->body_owner());
    }

    void visit(ForExpression* expression) override {
      accept(expression->body_owner());
      accept(expression->init_owner());
      accept(expression->last_owner());
      accept(expression->body_owner());
    }

    void visit(ReturnExpression* expression) override {
      if (auto value = expression->value_owner()) {
        accept(*value);
      }
    }

    void visit(BreakExpression* expression) override {
      if (auto value = expression->value_owner()) {
        accept(*value);
      }
    }

    void visit(ContinueExpression*) override {}

    void visit(BindingStatement* statement) override {
      if (auto hint = statement->hint_owner()) {
        accept(*hint);
      }

      accept(statement->initializer_owner());
    }

    void visit(ExpressionStatement* statement) override {
      accept(statement->expr_owner());
    }

    void visit(AssertStatement* statement) override {
      accept(statement->assertion_owner());
      accept(statement->message_owner());
    }

  protected:
    [[nodiscard]] std::unique_ptr<ast::Expression>* self_expr() noexcept {
      return expr_owner_;
    }

    [[nodiscard]] std::unique_ptr<ast::Statement>* self_stmt() noexcept {
      return stmt_owner_;
    }

    [[nodiscard]] std::unique_ptr<ast::Declaration>* self_decl() noexcept {
      return decl_owner_;
    }

    [[nodiscard]] std::unique_ptr<ast::Type>* self_type() noexcept {
      return type_owner_;
    }

    void replace_self(std::unique_ptr<ast::Expression> node) noexcept {
      *self_expr() = std::move(node);
    }

    void replace_self(std::unique_ptr<ast::Declaration> node) noexcept {
      *self_decl() = std::move(node);
    }

    void replace_self(std::unique_ptr<ast::Statement> node) noexcept {
      *self_stmt() = std::move(node);
    }

    void replace_self(std::unique_ptr<ast::Type> node) noexcept {
      *self_type() = std::move(node);
    }

  private:
    void accept(std::unique_ptr<ast::Expression>* expr) noexcept {
      expr_owner_ = expr;

      (*expr)->accept(this);
    }

    void accept(std::unique_ptr<ast::Statement>* stmt) noexcept {
      stmt_owner_ = stmt;

      (*stmt)->accept(this);
    }

    void accept(std::unique_ptr<ast::Declaration>* decl) noexcept {
      decl_owner_ = decl;

      (*decl)->accept(this);
    }

    void accept(std::unique_ptr<ast::Type>* type) noexcept {
      type_owner_ = type;

      (*type)->accept(this);
    }

    void accept_proto(ast::FnPrototype* proto) noexcept {
      for (auto& arg : proto->args_mut()) {
        accept(arg.type_owner());
      }

      accept(proto->return_type_owner());
    }

    std::unique_ptr<ast::Expression>* expr_owner_;
    std::unique_ptr<ast::Statement>* stmt_owner_;
    std::unique_ptr<ast::Declaration>* decl_owner_;
    std::unique_ptr<ast::Type>* type_owner_;
  };
} // namespace gal::ast
