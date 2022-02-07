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

#include "../ast/program.h"
#include "absl/types/span.h"
#include <optional>

namespace gal {
  /// Registers any Gallium builtin functions and `__builtin`s in the AST
  void register_predefined(ast::Program* program) noexcept;

  /// Tries to type-check a 'builtin', returns the type that it evaluates to if
  /// the call is valid, otherwise returns a nullopt
  std::optional<std::unique_ptr<ast::Type>> check_builtin(std::string_view name,
      absl::Span<const std::unique_ptr<ast::Expression>> args) noexcept;
} // namespace gal