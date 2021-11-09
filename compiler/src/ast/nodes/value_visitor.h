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
#include <optional>
#include <utility>

namespace gal::ast {
  /// Gives the operations for a "returning" visitor
  ///
  /// \tparam T The type to return
  /// \tparam BaseVisitor The visitor to derive from
  template <typename T, typename BaseVisitor> class ValueVisitor : public BaseVisitor {
  public:
    /// Gets the result from the value visitor, resets it to an "empty" state
    ///
    /// \return The value that the visitor "returned"
    T take_result() noexcept {
      assert(value_.has_value());

      return *std::exchange(value_, std::nullopt);
    }

    /// Gives a new "return value" for the visitor
    ///
    /// \tparam Args Arguments to in-place construct the return value with
    /// \param args The values to construct the value from
    template <typename... Args> void emplace(Args&&... args) noexcept {
      value_.emplace(std::forward<Args>(args)...);
    }

  private:
    std::optional<T> value_;
  };

  template <typename BaseVisitor> class ValueVisitor<void, BaseVisitor> : public BaseVisitor {
  public:
    void take_result() noexcept {}
  };
} // namespace gal::ast
