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
#include "absl/container/flat_hash_map.h"
#include "absl/flags/flag.h"
#include <optional>

ABSL_FLAG(std::string, opt, "none", "the optimization level to use (none|some|small|fast)");

ABSL_FLAG(std::string, emit, "exe", "the format to emit (ir|bc|asm|obj|lib|exe|graphviz)");

ABSL_FLAG(bool, verbose, false, "whether to enable verbose logging");

ABSL_FLAG(bool, debug, false, "whether or not to include debug information in the binary");

ABSL_FLAG(std::uint64_t, jobs, 1, "the number of threads that the compiler can create");

ABSL_FLAG(bool, colored, true, "whether or not to enable ANSI color codes in the compiler output");

namespace {
  std::optional<gal::OptLevel> parse_opt() noexcept {
    static absl::flat_hash_map<std::string_view, gal::OptLevel> lookup{
        {"none", gal::OptLevel::none},
        {"some", gal::OptLevel::some},
        {"small", gal::OptLevel::small},
        {"fast", gal::OptLevel::fast},
    };

    auto opt_level = absl::GetFlag(FLAGS_opt);
    auto it = lookup.find(std::string_view{opt_level});

    if (it == lookup.end()) {
      gal::errs() << "invalid value '" << opt_level
                  << "' for flag 'opt'! valid values: 'none', 'some', 'small', 'fast'";

      return std::nullopt;
    }

    return (*it).second;
  }

  std::optional<gal::OutputFormat> parse_emit() noexcept {
    static absl::flat_hash_map<std::string_view, gal::OutputFormat> lookup{
        {"ir", gal::OutputFormat::llvm_ir},
        {"bc", gal::OutputFormat::llvm_bc},
        {"asm", gal::OutputFormat::assembly},
        {"obj", gal::OutputFormat::object_code},
        {"lib", gal::OutputFormat::static_lib},
        {"exe", gal::OutputFormat::exe},
        {"graphviz", gal::OutputFormat::ast_graphviz},
    };

    auto emit = absl::GetFlag(FLAGS_emit);
    auto it = lookup.find(std::string_view{emit});

    if (it == lookup.end()) {
      gal::errs() << "invalid value '" << emit
                  << "' for flag 'emit'! valid values: 'ir', 'bc', 'asm', 'obj', 'lib', 'exe', 'graphviz'";

      return std::nullopt;
    }

    return (*it).second;
  }

  gal::CompilerConfig generate_config() noexcept {
    auto jobs = absl::GetFlag(FLAGS_jobs);
    auto debug = absl::GetFlag(FLAGS_debug);
    auto verbose = absl::GetFlag(FLAGS_verbose);
    auto colored = absl::GetFlag(FLAGS_colored);
    auto emit = parse_emit();
    auto opt = parse_opt();

    if (emit == std::nullopt || opt == std::nullopt) {
      std::abort();
    }

    return gal::CompilerConfig(jobs, *opt, *emit, debug, verbose, colored);
  }
} // namespace

namespace gal {
  CompilerConfig::CompilerConfig(std::uint64_t jobs,
      OptLevel opt,
      OutputFormat emit,
      bool debug,
      bool verbose,
      bool colored) noexcept
      : jobs_{jobs},
        opt_level_{opt},
        format_{emit},
        debug_{debug},
        verbose_{verbose},
        colored_{colored} {}

  const CompilerConfig& flags() noexcept {
    static CompilerConfig config = generate_config();

    return config;
  }

  std::ostream& operator<<(std::ostream& os, const CompilerConfig& flags) noexcept {
    os << "flags:"
       << " emit: " << static_cast<int>(flags.emit()) << ", opt: " << static_cast<int>(flags.opt())
       << ", jobs: " << flags.jobs() << ", debug: " << flags.debug() << ", verbose: " << flags.verbose();

    return os;
  }
} // namespace gal