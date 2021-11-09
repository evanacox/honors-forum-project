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

#include "../source_loc.h"
#include "absl/types/span.h"

namespace gal::ast {
  /// Base class for all AST nodes, contains source mapping information
  class Node {
  public:
    Node() = delete;

    /// Gets the source info for the node
    ///
    /// \return Source location information
    [[nodiscard]] const SourceLoc& loc() const noexcept {
      return loc_;
    }

  protected:
    /// Initializes the Node
    ///
    /// \param loc
    explicit Node(SourceLoc loc) noexcept : loc_{std::move(loc)} {};

  private:
    SourceLoc loc_;
  };

  namespace internal {
    /// Cast that does correctness checking in debug mode
    ///
    /// \param entity The entity to cast
    /// \return The cast result
    template <typename T, typename U> [[nodiscard]] constexpr T& debug_cast(U& entity) noexcept {
      assert(dynamic_cast<std::remove_reference_t<T>*>(&entity) != nullptr);

      return static_cast<T&>(entity);
    }

    struct GenericArgsCmp {
      template <typename T>
      [[nodiscard]] constexpr bool operator()(absl::Span<const std::unique_ptr<T>> lhs,
          absl::Span<const std::unique_ptr<T>> rhs) const noexcept {
        return std::equal(lhs.begin(), lhs.end(), rhs.begin(), rhs.end(), gal::DerefEq<>{});
      }

      template <typename T> [[nodiscard]] constexpr bool operator()(const T& lhs, const T& rhs) const noexcept {
        return (*this)(absl::MakeConstSpan(lhs), absl::MakeConstSpan(rhs));
      }
    };

    template <typename T>
    [[nodiscard]] std::vector<std::unique_ptr<T>> clone_generics(const std::vector<std::unique_ptr<T>>& generics) {
      auto vec = std::vector<std::unique_ptr<T>>{};

      for (auto& ptr : generics) {
        vec.push_back(ptr->clone());
      }

      return vec;
    }
  } // namespace internal
} // namespace gal::ast
