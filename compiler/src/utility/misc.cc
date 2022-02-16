//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "misc.h"
#include "absl/strings/charconv.h"
#include <charconv>
#include <cmath>

namespace {
  absl::chars_format into_abseil_format(std::chars_format format) noexcept {
    switch (format) {
      case std::chars_format::fixed: return absl::chars_format::fixed;
      case std::chars_format::scientific: return absl::chars_format::scientific;
      case std::chars_format::general: return absl::chars_format::general;
      case std::chars_format::hex: return absl::chars_format::hex;
      default: assert(false); return absl::chars_format::general;
    }
  }

  template <typename T> std::string reserve_for(T n, int base) {
    auto result = std::string{};

    switch (base) {
      case 2:
        if ((1 << result.capacity()) < n) {
          break;
        }

        return result;
    }
  }

  template <typename T> std::string generic_integral_to_digits(T n, int base) noexcept {
    assert(2 <= base && base <= 36);

    auto result = std::string{};
    result.resize(std::numeric_limits<std::uint64_t>::digits, ' ');

    auto [ptr, ec] = std::to_chars(result.data(), result.data() + result.size(), n, base);
    assert(ec == std::errc());
    result.resize(std::distance(result.data(), ptr));

    return result;
  }

  template <typename T> std::string generic_float_to_digits(T n, std::chars_format fmt) noexcept {
    auto result = std::string{};
    result.resize(256, ' ');

    // in reality there's no way to know how long these could
    // actually end up being, so we just start with a really big
    // number and double in size until we have enough space
    while (true) {
      auto [ptr, ec] = std::to_chars(result.data(), result.data() + result.size(), n, fmt);

      if (ec != std::errc::value_too_large) {
        result.resize(std::distance(result.data(), ptr));

        break;
      }

      result.resize(result.size() * 2, ' ');
    }

    return result;
  }
} // namespace

std::variant<std::uint64_t, std::error_code> gal::from_digits(std::string_view text, int base) noexcept {
  auto value = std::uint64_t{0};
  auto [_, ec] = std::from_chars(text.data(), text.data() + text.size(), value, base);

  return (ec != std::errc()) ? std::variant<std::uint64_t, std::error_code>{std::make_error_code(ec)}
                             : std::variant<std::uint64_t, std::error_code>{value};
}

std::variant<double, std::error_code> gal::from_digits(std::string_view text, std::chars_format format) noexcept {
  // from_chars on libstdc++ is screwy for floating-point, works on linux but not some
  // distributions of mingw-w64.
  //
  // since abseil provides it, may as well use that one for portability's sake (with integers
  // it doesn't really matter, but for floating-point ones we want reproducibility across standard libs)
  double value;
  auto [_, ec] = absl::from_chars(text.data(), text.data() + text.size(), value, into_abseil_format(format));

  return (ec != std::errc()) ? std::variant<double, std::error_code>{std::make_error_code(ec)}
                             : std::variant<double, std::error_code>{value};
}

std::string gal::to_digits(std::uint64_t n, int base) noexcept {
  return generic_integral_to_digits(n, base);
}

std::string gal::to_digits(std::int64_t n, int base) noexcept {
  return generic_integral_to_digits(n, base);
}

std::string gal::to_digits(double n, std::chars_format format) noexcept {
  return generic_float_to_digits(n, format);
}

std::string gal::to_digits(float n, std::chars_format format) noexcept {
  return generic_float_to_digits(n, format);
}
