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

#include "../../ast/program.h"
#include "../../ast/visitors.h"
#include "./constant_pool.h"
#include "./llvm_state.h"
#include "./stored_value.h"
#include "./variable_resolver.h"
#include "absl/container/flat_hash_map.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"

namespace gal::backend {
  /// Handles IR generation
  ///
  /// Visits the entire AST and generates code for it
  class CodeGenerator final : public ast::ConstDeclarationVisitor<void>,
                              public ast::ConstExpressionVisitor<backend::StoredValue>,
                              public ast::ConstStatementVisitor<backend::StoredValue> {
    using Expr = ast::ConstExpressionVisitor<backend::StoredValue>;
    using Stmt = ast::ConstStatementVisitor<backend::StoredValue>;

  public:
    explicit CodeGenerator(llvm::LLVMContext* context,
        const ast::Program& program,
        const llvm::TargetMachine& machine) noexcept;

    std::unique_ptr<llvm::Module> codegen() noexcept;

  protected:
    void visit(const ast::ImportDeclaration& declaration) final;

    void visit(const ast::ImportFromDeclaration& declaration) final;

    void visit(const ast::FnDeclaration& declaration) final;

    void visit(const ast::StructDeclaration& declaration) final;

    void visit(const ast::ClassDeclaration& declaration) final;

    void visit(const ast::TypeDeclaration& declaration) final;

    void visit(const ast::MethodDeclaration& declaration) final;

    void visit(const ast::ExternalFnDeclaration& declaration) final;

    void visit(const ast::ExternalDeclaration& declaration) final;

    void visit(const ast::ConstantDeclaration& declaration) final;

    void visit(const ast::StringLiteralExpression& expression) final;

    void visit(const ast::IntegerLiteralExpression& expression) final;

    void visit(const ast::FloatLiteralExpression& expression) final;

    void visit(const ast::BoolLiteralExpression& expression) final;

    void visit(const ast::CharLiteralExpression& expression) final;

    void visit(const ast::NilLiteralExpression& expression) final;

    void visit(const ast::ArrayExpression& expression) final;

    void visit(const ast::UnqualifiedIdentifierExpression& expression) final;

    void visit(const ast::IdentifierExpression& expression) final;

    void visit(const ast::StaticGlobalExpression& expression) final;

    void visit(const ast::LocalIdentifierExpression& expression) final;

    void visit(const ast::StructExpression& expression) final;

    void visit(const ast::CallExpression& expression) final;

    void visit(const ast::StaticCallExpression& expression) final;

    void visit(const ast::MethodCallExpression& expression) final;

    void visit(const ast::StaticMethodCallExpression& expression) final;

    void visit(const ast::IndexExpression& expression) final;

    void visit(const ast::FieldAccessExpression& expression) final;

    void visit(const ast::GroupExpression& expression) final;

    void visit(const ast::UnaryExpression& expression) final;

    void visit(const ast::BinaryExpression& expression) final;

    void visit(const ast::CastExpression& expression) final;

    void visit(const ast::SliceOfExpression& expression) final;

    void visit(const ast::RangeExpression& expression) final;

    void visit(const ast::IfThenExpression& expression) final;

    void visit(const ast::IfElseExpression& expression) final;

    void visit(const ast::BlockExpression& expression) final;

    void visit(const ast::LoopExpression& expression) final;

    void visit(const ast::WhileExpression& expression) final;

    void visit(const ast::ForExpression& expression) final;

    void visit(const ast::ReturnExpression& expression) final;

    void visit(const ast::BreakExpression& expression) final;

    void visit(const ast::ContinueExpression& expression) final;

    void visit(const ast::ImplicitConversionExpression& expression) final;

    void visit(const ast::LoadExpression& expression) final;

    void visit(const ast::AddressOfExpression& expression) final;

    void visit(const ast::SizeofExpression& expression) final;

    void visit(const ast::BindingStatement& statement) final;

    void visit(const ast::ExpressionStatement& statement) final;

    void visit(const ast::AssertStatement& statement) final;

  private:
    [[nodiscard]] llvm::Value* generate_mul(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_div(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_mod(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_add(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_sub(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_left_shift(const ast::Expression& expr,
        llvm::Value* lhs,
        llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_right_shift(const ast::Expression& expr,
        llvm::Value* lhs,
        llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_bit_and(const ast::Expression& expr,
        llvm::Value* lhs,
        llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_bit_or(const ast::Expression& expr,
        llvm::Value* lhs,
        llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Value* generate_bit_xor(const ast::Expression& expr,
        llvm::Value* lhs,
        llvm::Value* rhs) noexcept;

    [[nodiscard]] llvm::Function* codegen_proto(const ast::FnPrototype& proto, std::string_view name) noexcept;

    [[nodiscard]] backend::StoredValue codegen(const ast::Expression& expr) noexcept;

    [[nodiscard]] backend::StoredValue codegen_promoting(const ast::Expression& expr,
        llvm::Type* type = nullptr) noexcept;

    [[nodiscard]] llvm::Function* current_fn() noexcept;

    [[nodiscard]] std::string next_label() noexcept;

    [[nodiscard]] llvm::BasicBlock* panic_block() noexcept;

    [[nodiscard]] llvm::BasicBlock* assert_block() noexcept;

    [[nodiscard]] llvm::Value* integer_cast(std::uint32_t to_width,
        std::uint32_t from_width,
        bool is_signed,
        llvm::Value* value) noexcept;

    [[nodiscard]] llvm::Value* panic_if_overflow(const ast::SourceLoc& loc,
        std::string_view message,
        llvm::Intrinsic::IndependentIntrinsics intrin,
        llvm::Value* lhs,
        llvm::Value* rhs) noexcept;

    void panic_if(const ast::SourceLoc& loc, llvm::Value* cond, std::string_view message) noexcept;

    [[nodiscard]] llvm::Value* source_loc(const ast::SourceLoc& loc, std::string_view message) noexcept;

    [[nodiscard]] llvm::BasicBlock* create_block(std::string_view name = "", bool true_end = false) noexcept;

    void merge_with(llvm::BasicBlock* merge_block) noexcept;

    void emit_terminator() noexcept;

    void reset_fn_state() noexcept;

    [[nodiscard]] llvm::IRBuilder<>* builder() noexcept;

    const ast::Program& program_;
    LLVMState state_;
    ConstantPool pool_;
    VariableResolver variables_;
    llvm::BasicBlock* loop_start_ = nullptr;       // loop header, e.g the "check loop conditions and jump to body"
    llvm::BasicBlock* loop_merge_ = nullptr;       // loop merge point, used for breaks
    llvm::BasicBlock* exit_block_ = nullptr;       // the block that loads the return value and returns
    llvm::BasicBlock* dead_block_ = nullptr;       // the block that instructions generated after a terminator go
    llvm::BasicBlock* panic_block_ = nullptr;      // the block that panics with a message
    llvm::BasicBlock* assert_block_ = nullptr;     // the block that panics with a message
    llvm::PHINode* panic_phi_ = nullptr;           // the phi node that gets the panic message
    llvm::PHINode* assert_phi_ = nullptr;          // the phi node that gets the panic message
    llvm::AllocaInst* return_value_ = nullptr;     // the return value to be stored into
    llvm::AllocaInst* loop_break_value_ = nullptr; // the value of a loop to store into
    std::size_t curr_label_ = 1;
  };
} // namespace gal::backend
