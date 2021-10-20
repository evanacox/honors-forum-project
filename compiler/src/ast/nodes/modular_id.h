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
  /// Represents a module name, e.g `foo::bar::baz`
  class ModuleID {
  public:
    /// Creates a module id
    ///
    /// \param parts The parts that make up the name, i.e `{foo, bar, baz}` for `foo::bar::baz`
    explicit ModuleID(std::vector<std::string> parts) noexcept : parts_{std::move(parts)} {}

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
      return absl::StrJoin(parts_, "::");
    }

  private:
    std::vector<std::string> parts_;
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

  private:
    ModuleID module_;
    std::string id_;
  };
} // namespace gal::ast
