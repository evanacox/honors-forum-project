//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./llvm_state.h"

namespace gal::backend {
  LLVMState::LLVMState(llvm::LLVMContext* context,
      const llvm::TargetMachine& machine,
      [[maybe_unused]] const ast::Program& program) noexcept
      : context_{context},
        machine_{machine},
        layout_{machine_.createDataLayout()},
        module_{std::make_unique<llvm::Module>("main", *context_)},
        builder_{std::make_unique<llvm::IRBuilder<>>(*context_)} {
    module_->setTargetTriple(machine_.getTargetTriple().getTriple());
    module_->setDataLayout(layout_);
  }

  llvm::LLVMContext& LLVMState::context() noexcept {
    return *context_;
  }

  llvm::IRBuilder<>* LLVMState::builder() noexcept {
    return builder_.get();
  }

  llvm::Module* LLVMState::module() noexcept {
    return module_.get();
  }

  const llvm::DataLayout& LLVMState::layout() const noexcept {
    return layout_;
  }

  std::unique_ptr<llvm::Module> LLVMState::take_module() noexcept {
    return std::exchange(module_, {});
  }
} // namespace gal::backend