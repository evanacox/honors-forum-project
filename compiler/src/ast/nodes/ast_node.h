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

#include "../../utility/misc.h"
#include "../modular_id.h"
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

  /// Mixin type for nodes that need to be name-mangled
  class Mangled {
  public:
    /// Gets the mangled name of a functionf
    ///
    /// \return The mangled name of the function
    [[nodiscard]] std::string_view mangled_name() const noexcept {
      assert(mangled_.has_value());

      return *mangled_;
    }

    /// Gets the ID of the node
    ///
    /// \return The ID
    [[nodiscard]] const ast::FullyQualifiedID& id() const noexcept {
      assert(id_.has_value());

      return *id_;
    }

    /// Sets the "fully qualified" ID part of the entity
    ///
    /// \param id
    void set_id(FullyQualifiedID id) noexcept {
      id_ = std::move(id);
    }

    /// Sets the mangled name of a function
    ///
    /// \param mangled_name The mangled name of the function
    void set_mangled(std::string mangled_name) noexcept {
      mangled_ = std::move(mangled_name);
    }

  private:
    std::optional<FullyQualifiedID> id_;
    std::optional<std::string> mangled_;
  };

  namespace internal {
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
