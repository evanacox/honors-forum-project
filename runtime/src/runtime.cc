//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./runtime.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>

int gal::runtime::argc = 0;
char** gal::runtime::argv = nullptr;

extern "C" void gal::runtime::__gallium_assert_fail(const char* file, std::uint64_t line, const char* msg) noexcept {
  std::cerr << "gallium: assertion failure!\n";
  std::cerr << "  location: " << file << ", line: " << line << '\n';
  std::cerr << "  message: '" << msg << "'\n" << std::flush;

  runtime::__gallium_trap();
}

extern "C" void gal::runtime::__gallium_panic(const char* file, std::uint64_t line, const char* msg) noexcept {
  std::cerr << "gallium: panicked!\n";
  std::cerr << "  location: " << file << ", line: " << line << '\n';
  std::cerr << "  message: '" << msg << "'\n" << std::flush;

  runtime::__gallium_trap();
}