//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./flags.h"
#include "./log.h"
#include "absl/flags/flag.h"
#include <optional>

ABSL_FLAG(std::string, opt, "none", "the optimization level to use (none|some|small|fast)");

ABSL_FLAG(std::string, emit, "exe", "the format to emit (ir|bc|asm|obj|lib|exe|graphviz)");

ABSL_FLAG(bool, verbose, false, "whether to enable verbose logging");

ABSL_FLAG(bool, debug, false, "whether or not to include debug information in the binary");

ABSL_FLAG(std::uint64_t, jobs, 1, "the number of threads that the compiler can create");

namespace {
  std::optional<galc::OptLevel> optimization_level() noexcept {
    auto opt_level = absl::GetFlag(FLAGS_opt);

    if (opt_level == "none") {
      return galc::OptLevel::none;
    } else if (opt_level == "some") {
      return galc::OptLevel::some;
    } else if (opt_level == "small") {
      return galc::OptLevel::small;
    } else if (opt_level == "fast") {
      return galc::OptLevel::fast;
    }

    galc::errs() << "invalid value '" << opt_level << "' for flag 'opt'! valid values: 'none', 'some', 'small', 'fast'";

    return std::nullopt;
  }

  galc::CompilerConfig generate_config() noexcept {
    return galc::CompilerConfig(1, galc::OptLevel::none, galc::OutputFormat::assembly, false, false);
  }
} // namespace

namespace galc {
  CompilerConfig::CompilerConfig(std::uint64_t jobs, OptLevel opt, OutputFormat emit, bool debug, bool verbose) noexcept
      : jobs_{jobs},
        opt_level_{opt},
        format_{emit},
        debug_{debug},
        verbose_{verbose} {}

  const CompilerConfig& flags() noexcept {
    static CompilerConfig config = generate_config();

    return config;
  }
} // namespace galc