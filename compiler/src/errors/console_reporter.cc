//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./console_reporter.h"

namespace {
  //
}

namespace gal {
  ConsoleReporter::ConsoleReporter(std::ostream* os, std::string_view source) noexcept
      : gal::DiagnosticReporter{source},
        out_{os} {}

  void ConsoleReporter::internal_report(gal::Diagnostic diagnostic) noexcept {
    error_count_ += 1;

    *out_ << diagnostic.build(source()) << "\n\n";
  }

  bool ConsoleReporter::internal_had_error() const noexcept {
    return error_count_ != 0;
  }
} // namespace gal