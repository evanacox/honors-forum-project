//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./ast/program.h"
#include "./errors/reporter.h"
#include "absl/types/span.h"
#include <optional>
#include <string_view>
#include <vector>

namespace gal {
  /// "Drives" the compilation process. Gives options/settings to other modules,
  /// calls other code in the correct order, etc.
  class Driver {
  public:
    /// Runs the compiler and returns an exit code for the program
    ///
    /// \param files The file options given to the program
    [[nodiscard]] int start(absl::Span<std::string_view> files) noexcept;

    /// Parses a file, if it parses successfully it is added to `programs_`
    /// and a pointer is returned. Otherwise, nullopt is returned.
    ///
    /// \param path The path of the file
    /// \param source The source of the file
    /// \param reporter A diagnostic reporter
    /// \return A possible program
    [[nodiscard]] std::optional<ast::Program*> parse_file(std::filesystem::path path,
        std::string_view source,
        gal::DiagnosticReporter* reporter) noexcept;

  private:
    std::vector<ast::Program> programs_;
  };
} // namespace gal