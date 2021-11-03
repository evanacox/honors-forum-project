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
#include <optional>

namespace gal {
  /// Casts a unique pointer of `U` into a unique pointer of `T`.
  ///
  /// \note Does debug checking with `dynamic_cast` when `NDEBUG` is not defined
  /// \param ptr The pointer to cast
  /// \return The casted pointer
  template <typename T, typename U> std::unique_ptr<T> static_unique_cast(std::unique_ptr<U> ptr) noexcept {
    auto* real = ptr.release();

    assert(dynamic_cast<T*>(real) != nullptr);

    return std::unique_ptr<T>(static_cast<T*>(real));
  }

  /// Takes two optionals and compares for deep equality. If they both have a value,
  /// the pointers are dereferenced and compared for equality. Otherwise, their has-value states
  /// are compared.
  ///
  /// \param lhs The first optional to compare
  /// \param rhs The second optional to compare
  /// \return `(lhs && rhs) ? (**lhs == **rhs) : (lhs.has_value() == rhs.has_value())`
  template <typename T>
  [[nodiscard]] bool unwrapping_equal(std::optional<const T*> lhs, std::optional<const T*> rhs) noexcept {
    if (lhs.has_value() && rhs.has_value()) {
      return **lhs == **rhs;
    }

    return lhs.has_value() == rhs.has_value();
  }
} // namespace gal
