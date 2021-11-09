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

#include "../ast/program.h"
#include <ostream>

namespace gal {
  /// Prints a tree structure with ASCII that represents the full program
  ///
  /// \param program The program to print
  /// \return A string containing an ASCII tree structure
  std::string pretty_print(const ast::Program& program) noexcept;

  /// Print a representation of the program in a Graphviz-compatible format
  ///
  /// \param program The program to print
  /// \return A string containing graphviz code that can be written to a .dot file
  std::string graphviz_print(const ast::Program& program) noexcept;
} // namespace gal
