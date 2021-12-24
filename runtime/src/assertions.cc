//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./runtime.h"
#include <cstdio>
#include <cstdlib>

extern "C" void gal::runtime::__gallium_assert_fail(const char* file, std::uint64_t line, const char* msg) noexcept {
  std::fputs("gallium: assertion failure!\n", stderr);
  std::fprintf(stderr, "  location: %s:%lu\n", file, line);
  std::fprintf(stderr, "  message: '%s'\n", msg);

  runtime::__gallium_trap();
}