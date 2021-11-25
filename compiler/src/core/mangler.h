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

#include "../ast/nodes/declaration.h"
#include "../ast/program.h"
#include <string_view>

namespace gal {
  /// Gets the mangled identifier representing a particular entity. Must be
  /// of a mangle-able type, i.e a function / method / something that
  /// gets a symbol in LLVM
  ///
  /// \param node The node to mangle
  /// \return The mangled symbol representing the node
  std::string mangle(const ast::Declaration& node) noexcept;

  /// Annotates the entire AST for a program with mangled symbol names
  /// that can be used by later phases
  ///
  /// \param program The program to annotate
  void mangle_program(ast::Program* program) noexcept;
} // namespace gal
