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

#include "./diagnostics.h"
#include "./reporter.h"
#include <ostream>

namespace gal {
  class ConsoleReporter final : public DiagnosticReporter {
  public:
    explicit ConsoleReporter(std::ostream* os, std::string_view source) noexcept;

  protected:
    void internal_report(gal::Diagnostic diagnostic) noexcept final;

    [[nodiscard]] bool internal_had_error() const noexcept final;

  private:
    std::size_t error_count_ = 0;
    std::ostream* out_;
  };
} // namespace gal
