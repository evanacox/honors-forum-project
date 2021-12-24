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

#include "./llvm_state.h"

namespace gal::backend {
  /// Populates the LLVM module with any builtins that
  /// need to be emitted directly in the IR
  ///
  /// \param state The state containing the module
  void generate_builtins(LLVMState* state) noexcept;
} // namespace gal::backend