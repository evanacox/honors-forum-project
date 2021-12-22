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
#include "./generator/code_generator.h"
#include "absl/container/flat_hash_map.h"

namespace ast = gal::ast;

namespace gal {
  std::unique_ptr<llvm::Module> codegen(llvm::LLVMContext* context,
      const ast::Program& program,
      const llvm::TargetMachine& machine) noexcept {
    return backend::CodeGenerator(context, program, machine).codegen();
  }
} // namespace gal