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
#include "./program.h"
#include "./visitors/declaration_visitor.h"
#include "./visitors/expression_visitor.h"
#include "./visitors/statement_visitor.h"
#include "./visitors/type_visitor.h"

namespace gal::ast {
  template <typename T1, typename T2 = T1, typename T3 = T1, typename T4 = T1>
  class AnyVisitor : public DeclarationVisitor<T1>,
                     public ExpressionVisitor<T2>,
                     public StatementVisitor<T3>,
                     public TypeVisitor<T4> {};

  template <typename T1, typename T2 = T1, typename T3 = T1, typename T4 = T1>
  class AnyConstVisitor : public ConstDeclarationVisitor<T1>,
                          public ConstExpressionVisitor<T2>,
                          public ConstStatementVisitor<T3>,
                          public ConstTypeVisitor<T4> {};

  template <typename T1, typename T2 = T1, typename T3 = T1, typename T4 = T1>
  class AnyVisitorBase : public AnyVisitor<T1, T2, T3, T4> {
    using Self = AnyVisitorBase<T1, T2, T3, T4>;

  public:
    virtual void walk_ast(Program* program) {
      for (auto& decl : program->decls_mut()) {
        accept(&decl);
      }
    }

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

    void visit(ErrorType*) override {}

    void visit(UnsizedIntegerType*) override {}

    void visit(ArrayType* type) override {
      accept(type->element_type_owner());
    }

    void visit(IndirectionType* type) override {
      accept(type->produced_owner());
    }

    void visit(ImportDeclaration*) override {}

    void visit(ImportFromDeclaration*) override {}

    void visit(FnDeclaration* declaration) override {
      accept_proto(declaration->proto_mut());
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

    void visit(ArrayExpression* expression) override {
      for (auto& element : expression->elements_mut()) {
        accept(&element);
      }
    }

    void visit(UnqualifiedIdentifierExpression*) override {}

    void visit(IdentifierExpression*) override {}

    void visit(StaticGlobalExpression*) override {}

    void visit(LocalIdentifierExpression*) override {}

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

    void visit(StaticCallExpression* expression) override {
      for (auto& arg : expression->args_mut()) {
        accept(&arg);
      }
    }

    void visit(MethodCallExpression*) override {}

    void visit(StaticMethodCallExpression*) override {}

    void visit(IndexExpression* expression) override {
      accept(expression->callee_owner());

      for (auto& arg : expression->indices_mut()) {
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

    void visit(ImplicitConversionExpression* expression) override {
      accept(expression->expr_owner());
      accept(expression->cast_to_owner());
    }

    void visit(LoadExpression* expression) override {
      accept(expression->expr_owner());
    }

    void visit(AddressOfExpression* expression) override {
      accept(expression->expr_owner());
    }

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
    template <typename T> void visit_children(T* node) noexcept {
      if constexpr (std::is_base_of_v<Declaration, T>) {
        auto* self = self_decl_owner();

        Self::visit(node);

        decl_owner_ = self;
      } else if constexpr (std::is_base_of_v<Expression, T>) {
        auto* self = self_expr_owner();

        Self::visit(node);

        expr_owner_ = self;
      } else if constexpr (std::is_base_of_v<Statement, T>) {
        auto* self = self_stmt_owner();

        Self::visit(node);

        stmt_owner_ = self;
      } else {
        auto* self = self_type_owner();

        Self::visit(node);

        type_owner_ = self;
      }
    }

    [[nodiscard]] Expression* self_expr() noexcept {
      return expr_owner_->get();
    }

    [[nodiscard]] Statement* self_stmt() noexcept {
      return stmt_owner_->get();
    }

    [[nodiscard]] Declaration* self_decl() noexcept {
      return decl_owner_->get();
    }

    [[nodiscard]] Type* self_type() noexcept {
      return type_owner_->get();
    }

    [[nodiscard]] std::unique_ptr<Expression>* self_expr_owner() noexcept {
      return expr_owner_;
    }

    [[nodiscard]] std::unique_ptr<Statement>* self_stmt_owner() noexcept {
      return stmt_owner_;
    }

    [[nodiscard]] std::unique_ptr<Declaration>* self_decl_owner() noexcept {
      return decl_owner_;
    }

    [[nodiscard]] std::unique_ptr<Type>* self_type_owner() noexcept {
      return type_owner_;
    }

    void replace_self(std::unique_ptr<Expression> node) noexcept {
      *expr_owner_ = std::move(node);
    }

    void replace_self(std::unique_ptr<Declaration> node) noexcept {
      *decl_owner_ = std::move(node);
    }

    void replace_self(std::unique_ptr<Statement> node) noexcept {
      *stmt_owner_ = std::move(node);
    }

    void replace_self(std::unique_ptr<Type> node) noexcept {
      *type_owner_ = std::move(node);
    }

    void accept(std::unique_ptr<Expression>* expr) noexcept {
      expr_owner_ = expr;

      (*expr)->accept(this);
    }

    void accept(std::unique_ptr<Statement>* stmt) noexcept {
      stmt_owner_ = stmt;

      (*stmt)->accept(this);
    }

    void accept(std::unique_ptr<Declaration>* decl) noexcept {
      decl_owner_ = decl;

      (*decl)->accept(this);
    }

    void accept(std::unique_ptr<Type>* type) noexcept {
      type_owner_ = type;

      (*type)->accept(this);
    }

    void accept_proto(FnPrototype* proto) noexcept {
      for (auto& arg : proto->args_mut()) {
        accept(arg.type_owner());
      }

      accept(proto->return_type_owner());
    }

  private:
    std::unique_ptr<Expression>* expr_owner_{};
    std::unique_ptr<Statement>* stmt_owner_{};
    std::unique_ptr<Declaration>* decl_owner_{};
    std::unique_ptr<Type>* type_owner_{};
  };

  template <typename T1, typename T2 = T1, typename T3 = T1, typename T4 = T1>
  class AnyConstVisitorBase : public AnyConstVisitor<T1, T2, T3, T4> {
    using Self = AnyConstVisitorBase<T1, T2, T3, T4>;

  public:
    virtual void walk_ast(const Program& program) {
      for (auto& decl : program.decls()) {
        accept(decl);
      }
    }

    void visit(const ReferenceType& type) override {
      accept(type.referenced());
    }

    void visit(const SliceType& type) override {
      accept(type.sliced());
    }

    void visit(const PointerType& type) override {
      accept(type.pointed());
    }

    void visit(const BuiltinIntegralType&) override {}

    void visit(const BuiltinFloatType&) override {}

    void visit(const BuiltinByteType&) override {}

    void visit(const BuiltinBoolType&) override {}

    void visit(const BuiltinCharType&) override {}

    void visit(const UnqualifiedUserDefinedType&) override {}

    void visit(const UserDefinedType&) override {}

    void visit(const FnPointerType& type) override {
      for (auto& arg : type.args()) {
        accept(arg);
      }

      accept(type.return_type());
    }

    void visit(const UnqualifiedDynInterfaceType&) override {}

    void visit(const DynInterfaceType&) override {}

    void visit(const VoidType&) override {}

    void visit(const NilPointerType&) override {}

    void visit(const ErrorType&) override {}

    void visit(const UnsizedIntegerType&) override {}

    void visit(const ArrayType& array) override {
      accept(array.element_type());
    }

    void visit(const IndirectionType& type) override {
      accept(type.produced());
    }

    void visit(const ImportDeclaration&) override {}

    void visit(const ImportFromDeclaration&) override {}

    void visit(const FnDeclaration& declaration) override {
      accept_proto(declaration.proto());
      accept(declaration.body());
    }

    void visit(const StructDeclaration& declaration) override {
      for (auto& field : declaration.fields()) {
        accept(field.type());
      }
    }

    void visit(const ClassDeclaration&) override {}

    void visit(const TypeDeclaration& declaration) override {
      accept(declaration.aliased());
    }

    void visit(const MethodDeclaration&) override {}

    void visit(const ExternalFnDeclaration& declaration) override {
      accept_proto(declaration.proto());
    }

    void visit(const ExternalDeclaration& declaration) override {
      for (auto& fn : declaration.externals()) {
        accept(fn);
      }
    }

    void visit(const ConstantDeclaration& declaration) override {
      accept(declaration.hint());
      accept(declaration.initializer());
    }

    void visit(const StringLiteralExpression&) override {}

    void visit(const IntegerLiteralExpression&) override {}

    void visit(const FloatLiteralExpression&) override {}

    void visit(const BoolLiteralExpression&) override {}

    void visit(const CharLiteralExpression&) override {}

    void visit(const NilLiteralExpression&) override {}

    void visit(const ArrayExpression& expression) override {
      for (auto& element : expression.elements()) {
        accept(*element);
      }
    }

    void visit(const UnqualifiedIdentifierExpression&) override {}

    void visit(const IdentifierExpression&) override {}

    void visit(const StaticGlobalExpression&) override {}

    void visit(const StructExpression& expression) override {
      accept(expression.struct_type());

      for (auto& field : expression.fields()) {
        accept(field.init());
      }
    }

    void visit(const CallExpression& expression) override {
      accept(expression.callee());

      for (auto& arg : expression.args()) {
        accept(arg);
      }
    }

    void visit(const StaticCallExpression& expression) override {
      for (auto& arg : expression.args()) {
        accept(arg);
      }
    }

    void visit(const MethodCallExpression&) override {}

    void visit(const StaticMethodCallExpression&) override {}

    void visit(const IndexExpression& expression) override {
      accept(expression.callee());

      for (auto& arg : expression.indices()) {
        accept(arg);
      }
    }

    void visit(const FieldAccessExpression& expression) override {
      accept(expression.object());
    }

    void visit(const GroupExpression& expression) override {
      accept(expression.expr());
    }

    void visit(const UnaryExpression& expression) override {
      accept(expression.expr());
    }

    void visit(const BinaryExpression& expression) override {
      accept(expression.lhs());
      accept(expression.rhs());
    }

    void visit(const CastExpression& expression) override {
      accept(expression.castee());
      accept(expression.cast_to());
    }

    void visit(const ImplicitConversionExpression& expression) override {
      accept(expression.expr());
      accept(expression.cast_to());
    }

    void visit(const LoadExpression& expression) override {
      accept(expression.expr());
    }

    void visit(const AddressOfExpression& expression) override {
      accept(expression.expr());
    }

    void visit(const IfThenExpression& expression) override {
      accept(expression.condition());
      accept(expression.true_branch());
      accept(expression.false_branch());
    }

    void visit(const IfElseExpression& expression) override {
      accept(expression.condition());
      accept(expression.block());

      for (auto& elif : expression.elif_blocks()) {
        accept(elif.condition());
        accept(elif.block());
      }

      if (auto else_block = expression.else_block()) {
        accept(*else_block);
      }
    }

    void visit(const BlockExpression& expression) override {
      for (auto& stmt : expression.statements()) {
        accept(&stmt);
      }
    }

    void visit(const LoopExpression& expression) override {
      accept(expression.body());
    }

    void visit(const WhileExpression& expression) override {
      accept(expression.condition());
      accept(expression.body());
    }

    void visit(const ForExpression& expression) override {
      accept(expression.body());
      accept(expression.init());
      accept(expression.last());
      accept(expression.body());
    }

    void visit(const ReturnExpression& expression) override {
      if (auto value = expression.value()) {
        accept(*value);
      }
    }

    void visit(const BreakExpression& expression) override {
      if (auto value = expression.value()) {
        accept(*value);
      }
    }

    void visit(const ContinueExpression&) override {}

    void visit(const BindingStatement& statement) override {
      if (auto hint = statement.hint()) {
        accept(*hint);
      }

      accept(statement.initializer());
    }

    void visit(const ExpressionStatement& statement) override {
      accept(statement.expr());
    }

    void visit(const AssertStatement& statement) override {
      accept(statement.assertion());
      accept(statement.message());
    }

  protected:
    template <typename T> void visit_children(T* node) noexcept {
      Self::visit(node);
    }

  private:
    void accept(const Expression& expr) noexcept {
      expr.accept(this);
    }

    void accept(const Statement& stmt) noexcept {
      stmt.accept(this);
    }

    void accept(const Declaration& decl) noexcept {
      decl.accept(this);
    }

    void accept(const Type& type) noexcept {
      type.accept(this);
    }

    void accept_proto(const FnPrototype& proto) noexcept {
      for (auto& arg : proto.args()) {
        accept(arg.type());
      }

      accept(proto.return_type());
    }
  };
} // namespace gal::ast
