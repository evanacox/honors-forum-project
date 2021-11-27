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

#include "./diagnostics.h"
#include <cstddef>

namespace gal {
  /// Abstract interface for reporting errors
  class DiagnosticReporter {
  public:
    /// Reports a diagnostic by some method
    ///
    /// \param diagnostic The diagnostic to report
    void report(gal::Diagnostic diagnostic) noexcept;

    /// Reports a diagnostic, but just constructs in-place
    ///
    /// \param code The error code
    /// \param parts Parts of the diagnostic message
    void report_emplace(std::int64_t code, std::vector<std::unique_ptr<DiagnosticPart>> parts) noexcept;

    /// Checks if an **error** (not just a diagnostic) had been reported so far
    ///
    /// \return Whether or not an error has been reported
    [[nodiscard]] bool had_error() const noexcept;

    /// Gets the source code that the diagnostic reporter is operating on
    ///
    /// \return The source code
    [[nodiscard]] std::string_view source() const noexcept;

    /// Gets the number of diagnostics that
    ///
    /// \return
    [[nodiscard]] std::size_t count() const noexcept {
      return count_;
    }

  protected:
    virtual void internal_report(gal::Diagnostic diagnostic) noexcept = 0;

    [[nodiscard]] virtual bool internal_had_error() const noexcept = 0;

    explicit DiagnosticReporter(std::string_view source) noexcept;

    virtual ~DiagnosticReporter() = default;

  private:
    std::size_t count_ = 0;
    std::string_view source_;
  };
} // namespace gal
