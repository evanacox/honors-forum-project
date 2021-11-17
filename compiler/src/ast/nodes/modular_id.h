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

#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/types/span.h"
#include <string>
#include <vector>

namespace gal::ast {
  class UnqualifiedID;
  class FullyQualifiedID;

  /// Represents a module name, e.g `foo::bar::baz`
  class ModuleID {
  public:
    /// Creates a module id
    ///
    /// \param from_root Whether or not the module starts with `::`
    /// \param parts The parts that make up the name, i.e `{foo, bar, baz}` for `foo::bar::baz`
    explicit ModuleID(bool from_root, std::vector<std::string> parts) noexcept
        : from_root_{from_root},
          parts_{std::move(parts)} {}

    /// Checks if the user put `::` in front to specify that the module lookup
    /// starts at the global level
    ///
    /// \return True if `::` was at the beginning
    [[nodiscard]] bool from_root() const noexcept {
      return from_root_;
    }

    /// Gets the parts of the module name, i.e `{foo, bar, baz}` for `foo::bar::baz`
    ///
    /// \return The parts of the module name
    [[nodiscard]] absl::Span<const std::string> parts() const noexcept {
      return parts_;
    }

    /// Gets a string representation of the module name
    ///
    /// \return A string representation of the module name
    [[nodiscard]] std::string to_string() const noexcept {
      auto s = absl::StrJoin(parts_, "::");

      if (from_root_) {
        return "::" + s;
      } else {
        return s;
      }
    }

    /// Compares two ModuleIDs for equality
    ///
    /// \param lhs The first module id to compare
    /// \param rhs The second module id to compare
    /// \return Whether or not they are equal in every observable way
    [[nodiscard]] friend bool operator==(const ModuleID& lhs, const ModuleID& rhs) noexcept {
      auto lhs_p = lhs.parts();
      auto rhs_p = rhs.parts();

      return lhs.from_root() == rhs.from_root() // early return on `from_root` before we do expensive vec equality
             && std::equal(lhs_p.begin(), lhs_p.end(), rhs_p.begin(), rhs_p.end());
    }

  private:
    friend UnqualifiedID module_into_unqualified(ModuleID) noexcept;
    friend FullyQualifiedID module_into_qualified(ModuleID) noexcept;

    bool from_root_;
    std::vector<std::string> parts_;
  };

  class UnqualifiedID {
  public:
    /// Forms an unqualified ID
    ///
    /// \param mod A module prefix, if there is one
    /// \param id The name of the entity
    explicit UnqualifiedID(std::optional<ModuleID> mod, std::string id) noexcept
        : prefix_{std::move(mod)},
          id_{std::move(id)} {}

    /// Views the module prefix the identifier was declared with
    ///
    /// \return The entity's module
    [[nodiscard]] const std::optional<ModuleID>& prefix() const noexcept {
      return prefix_;
    }

    /// Gets the name of the entity
    ///
    /// \return The entity's name
    [[nodiscard]] std::string_view name() const noexcept {
      return id_;
    }

    /// Gets a string representation of the identifier
    ///
    /// \return A string representation of the identifier
    [[nodiscard]] std::string to_string() const noexcept {
      if (prefix().has_value()) {
        return absl::StrCat(prefix()->to_string(), "::", id_);
      } else {
        return id_;
      }
    }

    /// Compares two UnqualifiedID for equality
    ///
    /// \param lhs The first module id to compare
    /// \param rhs The second module id to compare
    /// \return Whether or not they are equal in every observable way
    [[nodiscard]] friend bool operator==(const UnqualifiedID& lhs, const UnqualifiedID& rhs) noexcept {
      // it's fairly likely that two IDs may import the same module but different things,
      // we may as well early return on that case instead of doing the expensive module cmp first
      return lhs.name() == rhs.name() && lhs.prefix() == rhs.prefix();
    }

  private:
    std::optional<ModuleID> prefix_;
    std::string id_;
  };

  /// Represents a fully-qualified identifier to some entity
  class FullyQualifiedID {
  public:
    /// Forms a fully-qualified ID
    ///
    /// \param mod The module the entity is a part of
    /// \param id The name of the entity
    explicit FullyQualifiedID(ModuleID mod, std::string id) noexcept : module_{std::move(mod)}, id_{std::move(id)} {}

    /// Views the module that the entity is a part of
    ///
    /// \return The entity's module
    [[nodiscard]] const ModuleID& module() const noexcept {
      return module_;
    }

    /// Gets the name of the entity
    ///
    /// \return The entity's name
    [[nodiscard]] std::string_view name() const noexcept {
      return id_;
    }

    /// Gets a string representation of the identifier
    ///
    /// \return A string representation of the identifier
    [[nodiscard]] std::string to_string() const noexcept {
      return absl::StrCat(module_.to_string(), "::", id_);
    }

    /// Compares two FullyQualifiedID for equality
    ///
    /// \param lhs The first module id to compare
    /// \param rhs The second module id to compare
    /// \return Whether or not they are equal in every observable way
    [[nodiscard]] friend bool operator==(const FullyQualifiedID& lhs, const FullyQualifiedID& rhs) noexcept {
      // it's fairly likely that two IDs may import the same module but different things,
      // we may as well early return on that case instead of doing the expensive module cmp first
      return lhs.name() == rhs.name() && lhs.module() == rhs.module();
    }

  private:
    ModuleID module_;
    std::string id_;
  };

  /// Transforms a moduleID into an unqualified identifier
  ///
  /// \param id The module to transform
  /// \return An unqualified ID
  inline UnqualifiedID module_into_unqualified(ModuleID id) noexcept {
    assert(!id.parts_.empty());

    auto vec = std::move(id.parts_);
    auto last = std::move(vec.back());
    vec.pop_back();

    auto module = vec.empty() ? std::nullopt : std::make_optional(ModuleID(id.from_root(), std::move(vec)));

    return UnqualifiedID(std::move(module), std::move(last));
  }

  /// Transforms a moduleID into a fully-qualified identifier
  ///
  /// \param id The module to transform
  /// \return A fully-qualified ID
  inline FullyQualifiedID module_into_qualified(ModuleID id) noexcept {
    assert(!id.parts_.empty());
    assert(id.from_root());

    auto vec = std::move(id.parts_);
    auto last = std::move(vec.back());
    vec.pop_back();

    return FullyQualifiedID(ModuleID(id.from_root(), std::move(vec)), std::move(last));
  }
} // namespace gal::ast
