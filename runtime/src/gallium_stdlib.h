//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#pragma once

#include <cstdint>

namespace gal::runtime {
  extern "C" void __gallium_print_f32(float x, int precision) noexcept;

  extern "C" void __gallium_print_f64(double x, int precision) noexcept;

  extern "C" void __gallium_print_int(std::int64_t x) noexcept;

  extern "C" void __gallium_print_uint(std::uint64_t x) noexcept;

  extern "C" void __gallium_print_char(std::uint8_t x) noexcept;

  extern "C" void __gallium_print_string(const char* data, std::size_t length) noexcept;

  extern "C" int __gallium_argc() noexcept;

  extern "C" char** __gallium_argv() noexcept;

  extern "C" std::int64_t __gallium_rand(std::int64_t lower, std::int64_t upper) noexcept;
} // namespace gal::runtime