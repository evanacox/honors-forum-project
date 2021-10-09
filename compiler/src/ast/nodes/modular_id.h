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
#include <string>
#include <vector>

namespace gal::ast {
  class ModuleID {
  public:
    explicit ModuleID(std::vector<std::string> parts) noexcept : parts_{std::move(parts)} {}

    [[nodiscard]] absl::Span<const std::string> parts() const noexcept {
      return parts_;
    }

  private:
    std::vector<std::string> parts_;
  };

  ///
  ///
  class ModularID {
  public:
    ///
    ///
    /// \param parts
    /// \param id
    explicit ModularID(ModuleID mod, std::string id) noexcept : module_{std::move(mod)}, id_{std::move(id)} {}

    [[nodiscard]] ModuleID module() const noexcept {
      return module_;
    }

    [[nodiscard]] std::string_view id() const noexcept {
      return id_;
    }

  private:
    ModuleID module_;
    std::string id_;
  };
} // namespace gal::ast
