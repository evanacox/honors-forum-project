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
#include <iomanip>
#include <iostream>

extern "C" void gal::runtime::__gallium_print_f32(float x, int precision) noexcept {
  auto flags = std::cout.flags();

  std::cout << std::setprecision(precision) << x << std::flush;

  std::cout.flags(flags);
}

extern "C" void gal::runtime::__gallium_print_f64(double x, int precision) noexcept {
  auto flags = std::cout.flags();

  std::cout << std::setprecision(precision) << x << std::flush;

  std::cout.flags(flags);
}

extern "C" void gal::runtime::__gallium_print_int(std::int64_t x) noexcept {
  std::cout << x << std::flush;
}

extern "C" void gal::runtime::__gallium_print_uint(std::uint64_t x) noexcept {
  std::cout << x << std::flush;
}

extern "C" void gal::runtime::__gallium_print_char(std::uint8_t x) noexcept {
  std::cout.put(static_cast<char>(x));
  std::cout.flush();
}

extern "C" void gal::runtime::__gallium_print_string(const char* data, std::size_t length) noexcept {
  std::cout.write(data, static_cast<std::streamsize>(length));
  std::cout.flush();
}
