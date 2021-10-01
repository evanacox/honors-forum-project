//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include <cstddef>

namespace gallium {
  /// The optimization level of the output, matters
  /// no matter what the output form is
  enum class OptLevel : unsigned char {
    /// No optimizations at all, just naive translation
    none,
    /// Basic optimizations without a huge time tradeoff at compile time are enabled
    optimized,
    /// Optimizations focus on reducing code size instead of generating the fastest code
    small,
    /// All reasonable optimizations are enabled, build time is not a concern
    fast
  };

  /// Defines what format the compiler will be outputting as its final product
  enum class OutputFormat : unsigned char {
    /// Human-readable LLVM IR, output to a text file
    llvm_ir,
    /// LLVM bitcode in a binary format, not human-readable but
    /// suitable to be plugged into other LLVM tools on the CLI
    llvm_bc,
    /// Outputs human-readable assembly code
    assembly,
    /// Outputs machine code, whether that be a `.o` file or an `.exe`
    machine_code,
    /// Outputs the AST into a Graphviz-compatible format, i.e a `.dot` file
    ast_graphviz,
  };

  /// Holds the configuration options for the
  /// entire compiler that were passed in from the
  /// command line
  class CompilerConfig {
  public:
    /// Gets the number of threads that the compiler is allowed to
    /// create to parse/compile/whatever
    ///
    /// \return The number of threads the compiler can create
    [[nodiscard]] constexpr std::size_t jobs() const noexcept {
      return jobs_;
    }

    /// Gets the optimization level that the compiler should output code at
    ///
    /// \return The requested optimization level
    [[nodiscard]] constexpr OptLevel opt_level() const noexcept {
      return opt_level_;
    }

    /// Gets the format that the user wants the compiler to generate
    ///
    /// \return The output format for the compiler
    [[nodiscard]] constexpr OutputFormat format() const noexcept {
      return format_;
    }

    /// Checks whether the user plans to debug the generated code
    ///
    /// \return Whether or not the user wants to debug the generated code
    [[nodiscard]] constexpr bool debug() const noexcept {
      return debug_;
    }

    /// Whether or not to enable verbose logging
    ///
    /// \return Whether or not verbose logging is enabled
    [[nodiscard]] constexpr bool verbose() const noexcept {
      return verbose_;
    }

  private:
    std::size_t jobs_;
    OptLevel opt_level_;
    OutputFormat format_;
    bool debug_;
    bool verbose_;
  };
} // namespace gallium