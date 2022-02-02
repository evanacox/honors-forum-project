//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./parse_errors.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include "antlr4-common.h"
#include "antlr4-runtime.h"
#include <filesystem>
#include <memory>

namespace ast = gal::ast;

namespace {
  std::unique_ptr<gal::DiagnosticPart> underline_for(ast::SourceLoc loc, gal::DiagnosticType type) noexcept {
    auto pointed_out = gal::PointedOut{std::move(loc), "", type, gal::UnderlineType::squiggly};

    return std::make_unique<gal::UnderlineList>(std::vector<gal::PointedOut>{std::move(pointed_out)});
  }

  std::unique_ptr<gal::DiagnosticPart> note(std::string message) noexcept {
    return std::make_unique<gal::SingleMessage>(std::move(message), gal::DiagnosticType::note);
  }
} // namespace

namespace gal {
  ParserErrorListener::ParserErrorListener(std::filesystem::path file, gal::DiagnosticReporter* reporter) noexcept
      : diagnostics_{reporter},
        file_{std::move(file)} {}

  void ParserErrorListener::syntaxError(antlr4::Recognizer*,
      antlr4::Token* token,
      size_t line,
      size_t col,
      const std::string& msg,
      std::exception_ptr) {
    auto diagnostics = std::vector<std::unique_ptr<gal::DiagnosticPart>>{};

    if (token != nullptr) {
      auto loc = ast::SourceLoc(token->getText(), line, col, file_);
      diagnostics.push_back(underline_for(std::move(loc), gal::DiagnosticType::error));
    }
    if (auto idx = msg.find('{'); idx != std::string::npos) {
      auto expected_tokens = std::string_view{msg}.substr(idx, msg.find('}') - idx);
      auto expected = absl::StrSplit(expected_tokens, ",");
      auto message =
          absl::StrCat("expected one of the following tokens: ", absl::StrJoin(expected, ",", [](auto* out, auto sv) {
            absl::StrAppend(out, "`", sv, "`");
          }));

      diagnostics.push_back(note(std::move(message)));
    } else {
      diagnostics.push_back(note(msg));
    }

    push_error(std::move(diagnostics));
  }

  void ParserErrorListener::push_error(std::vector<std::unique_ptr<gal::DiagnosticPart>> diagnostics) noexcept {
    diagnostics_->report_emplace(5, std::move(diagnostics));
  }
} // namespace gal