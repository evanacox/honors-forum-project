//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./codegen.h"
#include "../ast/visitors.h"
#include "./backend/code_generator.h"
#include "./backend/optimizer.h"
#include "llvm/IR/Verifier.h"

namespace ast = gal::ast;

namespace gal {
  std::unique_ptr<llvm::Module> codegen(llvm::LLVMContext* context,
      llvm::TargetMachine* machine,
      const ast::Program& program) noexcept {
    auto module = backend::CodeGenerator(context, program, *machine).codegen();

    backend::optimize(module.get(), machine);

    return module;
  }
} // namespace gal