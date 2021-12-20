//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "../ast/program.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"

namespace gal {
  /// Generates an LLVM IR module from an AST module
  ///
  /// \param context The context to use for the module being returned
  /// \param program The program to generate code for
  /// \param layout The data layout to compile for
  /// \return A new LLVM IR module
  std::unique_ptr<llvm::Module> codegen(llvm::LLVMContext* context,
      const ast::Program& program,
      const llvm::DataLayout& layout) noexcept;
} // namespace gal