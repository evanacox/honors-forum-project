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

#include "llvm/IR/Module.h"
#include "llvm/Target/TargetMachine.h"

namespace gal {
  /// Emits the compiler's generated code (or other format) into a file
  ///
  /// \param module The module to output
  /// \param machine The machine to use when outputting
  /// \return Returns false if output could not be emitted
  bool emit(llvm::Module* module, llvm::TargetMachine* machine) noexcept;
} // namespace gal
