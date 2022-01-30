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

#include "./llvm_state.h"
#include "absl/container/flat_hash_map.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Module.h"
#include <cstddef>
#include <string>
#include <string_view>

namespace gal::backend {
  class ConstantPool final : public ast::ConstTypeVisitor<llvm::Type*> {
  public:
    explicit ConstantPool(LLVMState* state) noexcept;

    [[nodiscard]] llvm::Constant* string_literal(std::string_view data) noexcept;

    [[nodiscard]] llvm::Value* c_string_literal(std::string_view data) noexcept;

    [[nodiscard]] llvm::Constant* constant(llvm::Type* type, const ast::Expression& expr) noexcept;

    [[nodiscard]] llvm::Type* map_type(const ast::Type& type) noexcept;

    // uint32_t because LLVM GEPs for field indices must be 32-bit constants
    [[nodiscard]] std::uint32_t field_index(const ast::UserDefinedType& type, std::string_view name) noexcept;

    [[nodiscard]] llvm::Constant* constant64(std::int64_t value) noexcept;

    [[nodiscard]] llvm::Constant* constant32(std::int32_t value) noexcept;

    [[nodiscard]] llvm::Constant* constant_of(std::int64_t width, std::int64_t value) noexcept;

    [[nodiscard]] llvm::Constant* zero(llvm::Type* type) noexcept;

    [[nodiscard]] llvm::Type* native_type() noexcept;

    [[nodiscard]] llvm::Type* integer_of_width(std::int64_t width) noexcept;

    [[nodiscard]] llvm::Type* pointer_to(llvm::Type* type) noexcept;

    [[nodiscard]] llvm::Type* slice_of(llvm::Type* type) noexcept;

    [[nodiscard]] llvm::Type* array_of(llvm::Type* type, std::uint64_t length) noexcept;

    [[nodiscard]] llvm::Type* source_info_type() noexcept;

  protected:
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

  private:
    llvm::SmallVector<std::pair<llvm::Type*, std::string_view>, 8> from_structure(
        const ast::StructDeclaration& decl) noexcept;

    void create_user_type_mapping(std::string_view full_name,
        llvm::ArrayRef<std::pair<llvm::Type*, std::string_view>> array) noexcept;

    llvm::Type* struct_from(std::string_view name,
        llvm::ArrayRef<std::pair<llvm::Type*, std::string_view>> array) noexcept;

    LLVMState* state_;
    std::size_t curr_str_ = 0;
    absl::flat_hash_map<std::string, llvm::Constant*> string_literals_;

    // user_types_ maps `::foo::bar::Baz` -> `%foo.bar.Baz = type { ... }`
    // field_names_ maps `::foo::bar::Baz` -> List<field name>, where the index of {field} is
    // the index of that field in `%foo.bar.Baz`
    absl::flat_hash_map<std::string, llvm::Type*> user_types_;
    absl::flat_hash_map<std::string, std::vector<std::string_view>> field_names_;
  };
} // namespace gal::backend