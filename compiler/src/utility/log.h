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

#include "absl/strings/str_cat.h"
#include <ostream>
#include <string_view>

namespace gal {
  namespace internal {
    void lock_console() noexcept;
    void unlock_console() noexcept;

    template <bool Flush> class NewlineOstream {
    public:
      NewlineOstream() = delete;

      explicit NewlineOstream(std::ostream* os) noexcept : os_{os} {
        internal::lock_console();
      }

      NewlineOstream(const NewlineOstream&) = delete;

      NewlineOstream(NewlineOstream&&) noexcept = default;

      NewlineOstream& operator=(const NewlineOstream&) = delete;

      NewlineOstream& operator=(NewlineOstream&&) = delete;

      ~NewlineOstream() {
        *os_ << '\n';

        if constexpr (Flush) {
          *os_ << std::flush;
        }

        internal::unlock_console();
      }

      template <typename T> NewlineOstream& operator<<(T&& entity) noexcept {
        *os_ << entity;

        return *this;
      }

    private:
      std::ostream* os_;
    };

    using BufferedFakeOstream = internal::NewlineOstream<false>;

    using UnbufferedFakeOstream = internal::NewlineOstream<true>;
  } // namespace internal

  namespace colors {
    /// Black ANSI code
    inline constexpr std::string_view code_black = "\u001b[30m";

    /// Red ANSI code
    inline constexpr std::string_view code_red = "\u001b[31m";

    /// Green ANSI code
    inline constexpr std::string_view code_green = "\u001b[32m";

    /// Yellow ANSI code
    inline constexpr std::string_view code_yellow = "\u001b[33m";

    /// Blue ANSI code
    inline constexpr std::string_view code_blue = "\u001b[34m";

    /// Magenta ANSI code
    inline constexpr std::string_view code_magenta = "\u001b[35m";

    /// Cyan ANSI code
    inline constexpr std::string_view code_cyan = "\u001b[36m";

    /// White ANSI code
    inline constexpr std::string_view code_white = "\u001b[37m";

    /// Reset ANSI code
    inline constexpr std::string_view code_reset = "\u001b[0m";

    /// Bright Black ANSI code
    inline constexpr std::string_view code_bold_black = "\u001b[30;1m";

    /// Bright Red ANSI code
    inline constexpr std::string_view code_bold_red = "\u001b[31;1m";

    /// Bright Green ANSI code
    inline constexpr std::string_view code_bold_green = "\u001b[32;1m";

    /// Bright Yellow ANSI code
    inline constexpr std::string_view code_bold_yellow = "\u001b[33;1m";

    /// Bright Blue ANSI code
    inline constexpr std::string_view code_bold_blue = "\u001b[34;1m";

    /// Bright Magenta ANSI code
    inline constexpr std::string_view code_bold_magenta = "\u001b[35;1m";

    /// Bright Cyan ANSI code
    inline constexpr std::string_view code_bold_cyan = "\u001b[36;1m";

    /// Bright White ANSI code
    inline constexpr std::string_view code_bold_white = "\u001b[37;1m";

#define COLOR_FUNC(color)                                                                                              \
  inline std::string color(std::string_view message) {                                                                 \
    return absl::StrCat(code_##color, message, code_reset);                                                            \
  }                                                                                                                    \
                                                                                                                       \
  inline std::string bold_##color(std::string_view message) {                                                          \
    return absl::StrCat(code_##color, message, code_reset);                                                            \
  }

    COLOR_FUNC(black)
    COLOR_FUNC(red)
    COLOR_FUNC(green)
    COLOR_FUNC(yellow)
    COLOR_FUNC(blue)
    COLOR_FUNC(magenta)
    COLOR_FUNC(cyan)
    COLOR_FUNC(white)

#undef COLOR_FUNC
  } // namespace colors

  /// Gets a wrapped up `std::ostream` that maps to `std::cout`. Automatically
  /// adds newlines whenever the message being printed is finished
  ///
  /// \return An unbuffered "ostream"
  internal::UnbufferedFakeOstream outs() noexcept;

  /// Gets a wrapped up `std::ostream` that maps to `std::cerr`. Automatically
  /// adds newlines whenever the message being printed is finished, and
  /// will flush whenever the message is finished.
  ///
  /// \return An unbuffered "ostream"
  internal::UnbufferedFakeOstream errs() noexcept;

  /// Gets a wrapped up `std::ostream` that maps to `std::cout`. Does not automatically
  /// add anything
  ///
  /// \return A real ostream
  std::ostream& raw_outs() noexcept;
} // namespace gal