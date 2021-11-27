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
#include "../errors/reporter.h"

namespace gal {
  /// Walks through the entire program AST, and annotates each node with type information
  /// that needs it. During this walk, types are checked and errors are generated if
  /// types are not correct.
  ///
  /// \param program A program to be type-annotated and type-checked.
  /// \param reporter The diagnostic reporter
  /// \return A list of errors if the program did not type-check, and nothing if it did
  bool type_check(ast::Program* program, gal::DiagnosticReporter* reporter) noexcept;
} // namespace gal
