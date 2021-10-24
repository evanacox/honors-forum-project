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
} // namespace gal::ast
