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

#include "./nodes.h"
#include "./visitors.h"
#include "absl/types/span.h"
#include <memory>
#include <vector>

namespace gal::ast {
  class Program {
  public:
    explicit Program(std::vector<std::unique_ptr<Declaration>> decls) noexcept : declarations_{std::move(decls)} {}

    [[nodiscard]] absl::Span<const std::unique_ptr<Declaration>> decls() const noexcept {
      return declarations_;
    }

    [[nodiscard]] absl::Span<std::unique_ptr<Declaration>> decls_mut() noexcept {
      return absl::MakeSpan(declarations_);
    }

  private:
    std::vector<std::unique_ptr<Declaration>> declarations_;
  };
} // namespace gal::ast
