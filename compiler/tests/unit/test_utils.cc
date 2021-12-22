//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "test_utils.h"

namespace ast = tests::ast;

std::unique_ptr<ast::Type> tests::void_type() noexcept {
  return std::make_unique<ast::VoidType>(ast::SourceLoc::nonexistent());
}

ast::FnPrototype tests::make_proto(std::string name,
    std::vector<ast::Argument> args,
    std::unique_ptr<ast::Type> return_type,
    std::vector<ast::Attribute> attributes,
    std::optional<ast::SelfType> self) noexcept {
  return ast::FnPrototype(std::move(name), self, std::move(args), std::move(attributes), std::move(return_type));
}

std::unique_ptr<ast::BlockExpression> tests::empty_block() noexcept {
  return std::make_unique<ast::BlockExpression>(ast::SourceLoc::nonexistent(),
      std::vector<std::unique_ptr<ast::Statement>>{});
}

std::unique_ptr<ast::FnDeclaration> tests::make_fn(ast::FnPrototype proto) noexcept {
  return std::make_unique<ast::FnDeclaration>(ast::SourceLoc::nonexistent(),
      false,
      false,
      std::move(proto),
      empty_block());
}

std::unique_ptr<ast::ConstantDeclaration> tests::make_const(std::string_view name,
    std::unique_ptr<ast::Type> type) noexcept {
  return std::make_unique<ast::ConstantDeclaration>(ast::SourceLoc::nonexistent(),
      false,
      std::string{name},
      std::move(type),
      std::make_unique<ast::ErrorExpression>());
}

ast::Argument tests::make_arg(std::unique_ptr<ast::Type> type) noexcept {
  return ast::Argument{ast::SourceLoc::nonexistent(), "", std::move(type)};
}

std::unique_ptr<ast::Type> tests::ptr(bool mut, std::unique_ptr<ast::Type> pointed_to) noexcept {
  return std::make_unique<ast::PointerType>(ast::SourceLoc::nonexistent(), mut, std::move(pointed_to));
}

std::unique_ptr<ast::Type> tests::ref(bool mut, std::unique_ptr<ast::Type> pointed_to) noexcept {
  return std::make_unique<ast::ReferenceType>(ast::SourceLoc::nonexistent(), mut, std::move(pointed_to));
}

std::unique_ptr<ast::Type> tests::integer(bool sign, int width) noexcept {
  return std::make_unique<ast::BuiltinIntegralType>(ast::SourceLoc::nonexistent(),
      sign,
      static_cast<ast::IntegerWidth>(width));
}

std::unique_ptr<ast::Type> tests::integer(bool sign) noexcept {
  return std::make_unique<ast::BuiltinIntegralType>(ast::SourceLoc::nonexistent(),
      sign,
      ast::IntegerWidth::native_width);
}

std::unique_ptr<ast::Type> tests::byte() noexcept {
  return std::make_unique<ast::BuiltinByteType>(ast::SourceLoc::nonexistent());
}

std::unique_ptr<ast::Type> tests::user_defined(std::string_view module, std::string_view name) noexcept {
  return std::make_unique<ast::UserDefinedType>(ast::SourceLoc::nonexistent(),
      nullptr,
      ast::FullyQualifiedID{std::string{module}, std::string{name}});
}

std::unique_ptr<ast::Type> tests::dyn_user_defined(std::string_view module, std::string_view name) noexcept {
  return std::make_unique<ast::DynInterfaceType>(ast::SourceLoc::nonexistent(),
      ast::FullyQualifiedID{std::string{module}, std::string{name}});
}

template <typename... Args>
std::unique_ptr<ast::Type> tests::fn_ptr(std::unique_ptr<ast::Type> return_type, Args... args) noexcept {
  if constexpr (sizeof...(args) > 0) {
    return std::make_unique<ast::FnPointerType>(ast::SourceLoc::nonexistent(),
        gal::into_list(std::move(args)...),
        std::move(return_type));
  } else {
    return std::make_unique<ast::FnPointerType>(ast::SourceLoc::nonexistent(),
        std::vector<std::unique_ptr<ast::Type>>{},
        std::move(return_type));
  }
}

std::unique_ptr<ast::Type> tests::slice_of(bool mut, std::unique_ptr<ast::Type> type) noexcept {
  return std::make_unique<ast::SliceType>(ast::SourceLoc::nonexistent(), mut, std::move(type));
}

std::unique_ptr<ast::Type> tests::array_of(std::uint64_t len, std::unique_ptr<ast::Type> type) noexcept {
  return std::make_unique<ast::ArrayType>(ast::SourceLoc::nonexistent(), len, std::move(type));
}

std::unique_ptr<ast::Type> tests::float_of(ast::FloatWidth width) noexcept {
  return std::make_unique<ast::BuiltinFloatType>(ast::SourceLoc::nonexistent(), width);
}

std::unique_ptr<ast::Type> tests::char_type() noexcept {
  return std::make_unique<ast::BuiltinCharType>(ast::SourceLoc::nonexistent());
}

std::unique_ptr<ast::Type> tests::bool_type() noexcept {
  return std::make_unique<ast::BuiltinBoolType>(ast::SourceLoc::nonexistent());
}
