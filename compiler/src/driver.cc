//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./driver.h"
#include "./utility/flags.h"
#include "./utility/log.h"

namespace gal {
  Driver::Driver() noexcept = default;

  int Driver::start(absl::Span<std::string_view> files) noexcept {
    return 0;
  }
} // namespace gal