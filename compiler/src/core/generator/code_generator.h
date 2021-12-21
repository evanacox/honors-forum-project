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
#include "./variable_resolver.h"
#include "absl/container/flat_hash_map.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace gal::backend {
  /// Handles IR generation
  ///
  /// Visits the entire AST and generates code for it
  class CodeGenerator final : public ast::AnyConstVisitor<void, llvm::Value*, void, llvm::Type*> {
    using Type = ast::ConstTypeVisitor<llvm::Type*>;
    using Expr = ast::ConstExpressionVisitor<llvm::Value*>;

  public:
    explicit CodeGenerator(llvm::LLVMContext* context,
        const ast::Program& program,
        const llvm::DataLayout& layout) noexcept;

    std::unique_ptr<llvm::Module> codegen() noexcept;

    llvm::Function* codegen_proto(const ast::FnPrototype& proto, std::string_view name) noexcept;

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

    void visit(const ast::ReferenceType& type) final;

    void visit(const ast::SliceType& type) final;

    void visit(const ast::PointerType& type) final;

    void visit(const ast::BuiltinIntegralType& type) final;

    void visit(const ast::BuiltinFloatType& type) final;

    void visit(const ast::BuiltinByteType& type) final;

    void visit(const ast::BuiltinBoolType& type) final;

    void visit(const ast::BuiltinCharType& type) final;

    void visit(const ast::UnqualifiedUserDefinedType& type) final;

    void visit(const ast::UserDefinedType& type) final;

    void visit(const ast::FnPointerType& type) final;

    void visit(const ast::UnqualifiedDynInterfaceType& type) final;

    void visit(const ast::DynInterfaceType& type) final;

    void visit(const ast::VoidType& type) final;

    void visit(const ast::NilPointerType& type) final;

    void visit(const ast::ErrorType& type) final;

    void visit(const ast::UnsizedIntegerType& type) final;

    void visit(const ast::ArrayType& type) final;

    void visit(const ast::IndirectionType& type) final;

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

    void visit(const ast::BindingStatement& statement) final;

    void visit(const ast::ExpressionStatement& statement) final;

    void visit(const ast::AssertStatement& statement) final;

    llvm::Constant* generate_string_literal(const ast::StringLiteralExpression& literal) noexcept;

    std::string label_name() noexcept;

    void reset_label() noexcept;

    llvm::Type* native_type() noexcept;

    llvm::Type* integer_of_width(std::int64_t width) noexcept;

    llvm::Type* pointer_to(llvm::Type* type) noexcept;

    llvm::Type* slice_of(llvm::Type* type) noexcept;

    llvm::Type* map_type(const ast::Type& type) noexcept;

    llvm::Type* struct_for(const ast::UserDefinedType& type) noexcept;

    llvm::Type* array_of(llvm::Type* type, std::uint64_t length) noexcept;

    // int32_t because LLVM GEPs for field indices must be 32-bit constants
    std::int32_t field_index(const ast::UserDefinedType& type, std::string_view name) noexcept;

  private:
    llvm::SmallVector<std::pair<llvm::Type*, std::string_view>, 8> from_structure(
        const ast::StructDeclaration& decl) noexcept;

    void create_user_type_mapping(std::string_view full_name,
        llvm::ArrayRef<std::pair<llvm::Type*, std::string_view>> array) noexcept;

    llvm::Type* struct_from(llvm::ArrayRef<std::pair<llvm::Type*, std::string_view>> array) noexcept;

    llvm::Constant* into_constant(llvm::Type* type, const ast::Expression& expr) noexcept;

    llvm::Constant* int64_constant(std::int64_t value) noexcept;

    llvm::Constant* int32_constant(std::int32_t value) noexcept;

    llvm::Value* codegen_into_reg(const ast::Expression& expr, llvm::Type* type) noexcept;

    llvm::Value* codegen_into_reg(const ast::Expression& expr) noexcept;

    const ast::Program& program_;
    const llvm::DataLayout& layout_;
    std::size_t curr_label_ = 0;
    std::size_t curr_str_ = 0;
    llvm::LLVMContext* context_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
    VariableResolver variables_;
    absl::flat_hash_map<std::string, llvm::Constant*> string_literals_;
    absl::flat_hash_map<std::string, llvm::Type*> user_types_; // map of `struct name -> LLVM struct`
    absl::flat_hash_map<std::string, std::vector<std::string_view>>
        user_type_mapping_; // `struct name -> [field name]`, the index of the field name = index in the llvm type
    bool want_loaded_ = false;
  };
} // namespace gal::backend
