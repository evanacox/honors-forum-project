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

#include "../ast/nodes.h"
#include "../ast/program.h"
#include "../errors/reporter.h"
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace gal {
  /// Parses `text` and returns an AST from it, if there were no errors.
  /// If there were errors, they will be printed and `nullopt` is returned.
  ///
  /// \param file The file being parsed
  /// \param text The text to parse
  /// \return A possible AST
  std::optional<ast::Program> parse(std::filesystem::path file,
      std::string_view text,
      DiagnosticReporter* reporter) noexcept;
} // namespace gal