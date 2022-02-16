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

#include "../errors/diagnostics.h"
#include "../errors/reporter.h"
#include <antlr4-runtime.h>
#include <filesystem>
#include <optional>

namespace gal {
  /// Helps to improve general ANTLR parse errors from the terrible default
  /// that they produce
  class ParserErrorListener final : public antlr4::BaseErrorListener {
  public:
    /// Creates a ParserErrorListener.
    ///
    /// \param file The file being parsed
    explicit ParserErrorListener(std::filesystem::path file, gal::DiagnosticReporter* reporter) noexcept;

    void syntaxError(antlr4::Recognizer* recognizer,
        antlr4::Token* token,
        size_t line,
        size_t col,
        const std::string&,
        std::exception_ptr) final;

  private:
    /// Pushes an error into the internal error list
    ///
    /// \param diagnostics The parts of the diagnostic message
    void push_error(std::vector<std::unique_ptr<gal::DiagnosticPart>> diagnostics) noexcept;

    gal::DiagnosticReporter* diagnostics_;
    std::filesystem::path file_;
  };
} // namespace gal
