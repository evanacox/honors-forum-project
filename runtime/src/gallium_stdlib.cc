//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./gallium_stdlib.h"
#include "./runtime.h"
#include <iomanip>
#include <iostream>
#include <random>

extern "C" void gal::runtime::__gallium_print_f32(float x, int precision) noexcept {
  std::cout << std::fixed << std::setprecision(precision) << x;
}

extern "C" void gal::runtime::__gallium_print_f64(double x, int precision) noexcept {
  std::cout << std::fixed << std::setprecision(precision) << x;
}

extern "C" void gal::runtime::__gallium_print_int(std::int64_t x) noexcept {
  std::cout << x;
}

extern "C" void gal::runtime::__gallium_print_uint(std::uint64_t x) noexcept {
  std::cout << x;
}

extern "C" void gal::runtime::__gallium_print_char(std::uint8_t x) noexcept {
  std::cout.put(static_cast<char>(x));
}

extern "C" void gal::runtime::__gallium_print_string(const char* data, std::size_t length) noexcept {
  std::cout.write(data, static_cast<std::streamsize>(length));
}

extern "C" int gal::runtime::__gallium_argc() noexcept {
  return gal::runtime::argc;
}

extern "C" char** gal::runtime::__gallium_argv() noexcept {
  return gal::runtime::argv;
}

extern "C" std::int64_t gal::runtime::__gallium_rand(std::int64_t lower, std::int64_t upper) noexcept {
  static std::random_device rd{};
  static std::mt19937_64 rng{rd()};

  return std::uniform_int_distribution<std::int64_t>{lower, upper}(rng);
}
