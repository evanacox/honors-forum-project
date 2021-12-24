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

#include <cstdint>

namespace gal::runtime {
  /// This is the symbol that's generated by the Gallium compiler as the
  /// real "entry point" of the program. This is the symbol generated
  /// for the function `fn ::main() -> i32`
  extern "C" std::int32_t __gallium_user_main() noexcept;

  /// Called whenever the Gallium runtime (or executable itself) decides it
  /// has hit an unrecoverable error of some sort
  ///
  /// \param file String of the filename that the assertion came from
  /// \param line The line number of the assertion
  /// \param msg The message to display before aborting
  extern "C" [[noreturn]] void __gallium_panic(const char* file, std::uint64_t line, const char* msg) noexcept;

  /// Called whenever the Gallium executable itself fails an assertion
  ///
  /// \param file String of the filename that the assertion came from
  /// \param line The line number of the assertion
  /// \param msg The message to display before aborting
  extern "C" [[noreturn]] void __gallium_assert_fail(const char* file, std::uint64_t line, const char* msg) noexcept;

  /// Effectively `__builtin_trap` but emitted by LLVM directly
  extern "C" [[noreturn]] void __gallium_trap() noexcept;
} // namespace gal::runtime