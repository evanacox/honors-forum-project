//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "driver.h"

namespace gallium {
  Driver::Driver(int argc, char** argv) : argc_{argc}, argv_{argv} {}

  int Driver::start() noexcept {
    // ...

    return 0;
  }
} // namespace gallium