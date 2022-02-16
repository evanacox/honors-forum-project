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

#include "./llvm_state.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/Value.h"

namespace gal::backend {
  /// Populates the LLVM module with any builtins that
  /// need to be emitted directly in the IR
  ///
  /// \param state The state containing the module
  void generate_builtins(LLVMState* state) noexcept;

  /// Generates code to correctly "call" a compiler intrinsic
  ///
  /// \param name The name of the builtin being called
  /// \param state The current LLVM state
  /// \param args The arguments that were being passed to the builtin
  llvm::Value* call_builtin(std::string_view name, LLVMState* state, llvm::ArrayRef<llvm::Value*> args) noexcept;
} // namespace gal::backend