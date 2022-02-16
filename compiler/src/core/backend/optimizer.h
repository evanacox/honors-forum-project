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

#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"

namespace gal::backend {
  /// Puts `module` through the LLVM optimization pipeline
  /// as requested in the CLI flags
  ///
  /// \param module The module to optimize
  /// \param machine The machine to target for
  void optimize(llvm::Module* module, llvm::TargetMachine* machine) noexcept;
} // namespace gal::backend