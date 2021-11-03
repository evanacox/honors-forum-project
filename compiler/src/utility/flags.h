//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include <cstdint>
#include <ostream>

namespace gal {
  /// The optimization level of the output, matters
  /// no matter what the output form is
  enum class OptLevel : signed char {
    /// No optimizations at all, just naive translation
    none = -1,
    /// Basic optimizations without a huge time tradeoff at compile time are enabled
    some = -2,
    /// Optimizations focus on reducing code size instead of generating the fastest code
    small = -3,
    /// All reasonable optimizations are enabled, build time is not a concern
    fast = -4
  };

  /// Defines what format the compiler will be outputting as its final product
  enum class OutputFormat : signed char {
    /// Human-readable LLVM IR, output to a text file
    llvm_ir = -1,
    /// LLVM bitcode in a binary format, not human-readable but
    /// suitable to be plugged into other LLVM tools on the CLI
    llvm_bc = -2,
    /// Outputs human-readable assembly code
    assembly = -3,
    /// Outputs machine code in the form of a `.o` equivalent
    object_code = -4,
    /// Outputs machine code to a static library
    static_lib = -5,
    /// Outputs an executable that can be run
    exe = -6,
    /// Outputs the AST into a Graphviz-compatible format, i.e a `.dot` file
    ast_graphviz = -7,
  };

  /// Holds the configuration options for the
  /// entire compiler that were passed in from the
  /// command line
  class CompilerConfig {
  public:
    /// Create a CompilerConfig object
    explicit CompilerConfig(std::uint64_t jobs,
        OptLevel opt,
        OutputFormat emit,
        bool debug,
        bool verbose,
        bool colored) noexcept;

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
    [[nodiscard]] constexpr OptLevel opt() const noexcept {
      return opt_level_;
    }

    /// Gets the format that the user wants the compiler to generate
    ///
    /// \return The output format for the compiler
    [[nodiscard]] constexpr OutputFormat emit() const noexcept {
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

    [[nodiscard]] constexpr bool colored() const noexcept {
      return colored_;
    }

  private:
    std::uint64_t jobs_;
    OptLevel opt_level_;
    OutputFormat format_;
    bool debug_;
    bool verbose_;
    bool colored_;
  };

  /// Pretty-prints the flag state
  ///
  /// \param os The stream to print to
  /// \param config The configuration
  /// \return os
  std::ostream& operator<<(std::ostream& os, const CompilerConfig& config) noexcept;

  /// Parses the command line flags for the compiler and returns a config object
  ///
  /// \param argc The number of arguments
  /// \param argv The vector of arguments as C-strings
  /// \return A config object
  [[nodiscard]] const CompilerConfig& flags() noexcept;
} // namespace gal