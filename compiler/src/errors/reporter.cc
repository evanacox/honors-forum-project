//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./reporter.h"

namespace gal {
  DiagnosticReporter::DiagnosticReporter(std::string_view source) noexcept : source_{source} {}

  void DiagnosticReporter::report(gal::Diagnostic diagnostic) noexcept {
    internal_report(std::move(diagnostic));
  }

  void DiagnosticReporter::report_emplace(std::int64_t code,
      std::vector<std::unique_ptr<DiagnosticPart>> parts) noexcept {
    report(gal::Diagnostic{code, std::move(parts)});
  }

  bool DiagnosticReporter::had_error() const noexcept {
    return internal_had_error();
  }

  std::string_view DiagnosticReporter::source() const noexcept {
    return source_;
  }
} // namespace gal