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

#include "absl/types/span.h"
#include <cassert>
#include <functional>
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

  /// Equality functor that dereferences its arguments and performs `==` on them.
  ///
  /// \tparam T The type to compare
  template <typename T = void> struct DerefEq {
    constexpr bool operator()(const T& lhs, const T& rhs) const {
      return *lhs == *rhs;
    }
  };

  template <> struct DerefEq<void> {
    template <typename T, typename U> constexpr bool operator()(const T& lhs, const U& rhs) const {
      return *lhs == *rhs;
    }
  };

  /// Takes two optionals and compares for deep equality. If they both have a value,
  /// the optionals are unwrapped and compared using a functor. Otherwise, their has-value states
  /// are compared.
  ///
  /// \param lhs The first optional to compare
  /// \param rhs The second optional to compare
  /// \return `(lhs && rhs) ? (cmp(*lhs, *rhs)) : (lhs.has_value() == rhs.has_value())`
  template <typename T, typename U, typename Cmp = std::equal_to<T>>
  [[nodiscard]] bool unwrapping_equal(std::optional<T> lhs, std::optional<U> rhs, const Cmp& cmp = Cmp{}) noexcept {
    if (lhs.has_value() && rhs.has_value()) {
      return cmp(*lhs, *rhs);
    }

    return lhs.has_value() == rhs.has_value();
  }

  /// Clones a span over cloneable AST nodes and returns a new vector with
  /// the cloned nodes
  ///
  /// \param array The span to clone
  /// \return A new array containing each cloned element
  template <typename T>
  std::vector<std::unique_ptr<T>> clone_span(absl::Span<const std::unique_ptr<T>> array) noexcept {
    auto new_array = std::vector<std::unique_ptr<T>>{};

    for (auto& object : array) {
      new_array.push_back(object->clone());
    }

    return new_array;
  }

  /// Equivalent to `some.map(|val| val->clone())`
  ///
  /// \param maybe The maybe to map
  /// \return The mapped maybe
  template <typename T>
  std::optional<std::unique_ptr<T>> clone_if(const std::optional<std::unique_ptr<T>>& maybe) noexcept {
    if (maybe.has_value()) {
      return gal::static_unique_cast<T>((*maybe)->clone());
    }

    return std::nullopt;
  }
} // namespace gal
