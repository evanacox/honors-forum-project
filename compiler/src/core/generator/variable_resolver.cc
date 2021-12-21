//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./variable_resolver.h"
#include "../../utility/misc.h"

namespace gal::backend {
  VariableResolver::VariableResolver(llvm::IRBuilder<>* builder) noexcept : builder_{builder} {}

  llvm::Value* VariableResolver::get(std::string_view name) noexcept {
    for (auto& scope : gal::reverse(scopes_)) {
      if (auto it = scope.find(name); it != scope.end()) {
        return it->second;
      }
    }

    assert(false);

    return nullptr;
  }

  void VariableResolver::set(std::string_view name, llvm::Value* value) noexcept {
    assert(!scopes_.empty());

    scopes_.back().emplace(std::string{name}, value);
    builder_->CreateLifetimeStart(value);
  }

  void VariableResolver::enter_scope() noexcept {
    scopes_.emplace_back();
  }

  void VariableResolver::leave_scope() noexcept {
    auto& scope = scopes_.back();

    for (auto& [_, value] : scope) {
      builder_->CreateLifetimeEnd(value);
    }
  }
} // namespace gal::backend