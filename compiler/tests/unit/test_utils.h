//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#pragma once

#include "src/ast/nodes.h"
#include <memory>

namespace tests {
  namespace ast = gal::ast;

  std::unique_ptr<ast::Type> void_type() noexcept;

  ast::FnPrototype make_proto(std::string name,
      std::vector<ast::Argument> args = {},
      std::unique_ptr<ast::Type> return_type = void_type(),
      std::vector<ast::Attribute> attributes = {},
      std::optional<ast::SelfType> self = {}) noexcept;

  std::unique_ptr<ast::BlockExpression> empty_block() noexcept;

  std::unique_ptr<ast::FnDeclaration> make_fn(ast::FnPrototype proto) noexcept;

  std::unique_ptr<ast::ConstantDeclaration> make_const(std::string_view name, std::unique_ptr<ast::Type> type) noexcept;

  ast::Argument make_arg(std::unique_ptr<ast::Type> type) noexcept;

  std::unique_ptr<ast::Type> ptr(bool mut, std::unique_ptr<ast::Type> pointed_to) noexcept;

  std::unique_ptr<ast::Type> ref(bool mut, std::unique_ptr<ast::Type> pointed_to) noexcept;

  std::unique_ptr<ast::Type> integer(bool sign, int width) noexcept;

  std::unique_ptr<ast::Type> integer(bool sign) noexcept;

  std::unique_ptr<ast::Type> byte() noexcept;

  std::unique_ptr<ast::Type> user_defined(std::string_view module, std::string_view name) noexcept;

  std::unique_ptr<ast::Type> dyn_user_defined(std::string_view module, std::string_view name) noexcept;

  template <typename... Args>
  std::unique_ptr<ast::Type> fn_ptr(std::unique_ptr<ast::Type> return_type, Args... args) noexcept;

  std::unique_ptr<ast::Type> slice_of(bool mut, std::unique_ptr<ast::Type> type) noexcept;

  std::unique_ptr<ast::Type> array_of(std::uint64_t len, std::unique_ptr<ast::Type> type) noexcept;

  std::unique_ptr<ast::Type> float_of(ast::FloatWidth width) noexcept;

  std::unique_ptr<ast::Type> char_type() noexcept;

  std::unique_ptr<ast::Type> bool_type() noexcept;
} // namespace tests