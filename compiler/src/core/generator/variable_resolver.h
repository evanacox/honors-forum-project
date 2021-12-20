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

#include "absl/container/flat_hash_map.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

namespace gal::backend {
  /// Resolves variables, generates IR in order to resolve when necessary
  class VariableResolver {
  public:
    /// Creates a variable table
    ///
    /// \param builder
    explicit VariableResolver(llvm::IRBuilder<>* builder) noexcept;

    /// Gets an IR value that resolves to the value at `name`
    ///
    /// If necessary, instructions will be generated to load/store as needed
    ///
    /// \param name The variable name to resolve
    /// \return An LLVM value that maps to the value
    [[nodiscard]] llvm::Value* get(std::string_view name) noexcept;

    /// Sets a variable in the nearest scope
    ///
    /// \param name The name
    /// \param value The LLVM value for the **address** of the variable (i.e the result of `alloca`)
    void set(std::string_view name, llvm::Value* value) noexcept;

    /// Sets a parameter with its name and the value mapping to it
    ///
    /// \param name The name
    /// \param value The LLVM value mapping to the parameter
    void set_param(std::string_view name, llvm::Value* value) noexcept;

    /// Enters a scope, changes where `set`/`get` look for normally
    void enter_scope() noexcept;

    /// Leaves a scope, ends the lifetime of all variables in the current scope
    /// and changes where future `get`s will return
    void leave_scope() noexcept;

    /// Deletes all the parameters
    void clear_params() noexcept;

  private:
    llvm::IRBuilder<>* builder_;
    std::vector<absl::flat_hash_map<std::string, llvm::Value*>> scopes_;
    absl::flat_hash_map<std::string, llvm::Value*> parameters_;
  };
} // namespace gal::backend
