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

#include <cassert>
#include <memory>

namespace gal {
  template <typename T, typename U> std::unique_ptr<T> static_unique_cast(std::unique_ptr<U> ptr) noexcept {
    auto* real = ptr.release();

    assert(dynamic_cast<T*>(real) != nullptr);

    return std::unique_ptr<T>(static_cast<T*>(real));
  }
} // namespace gal
