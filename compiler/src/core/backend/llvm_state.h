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

#include "../../ast/program.h"
#include "absl/container/flat_hash_map.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/Target/TargetMachine.h"
#include <string>
#include <string_view>

namespace gal::backend {
  class LLVMState {
  public:
    explicit LLVMState(llvm::LLVMContext* context,
        const llvm::TargetMachine& machine,
        const ast::Program& program) noexcept;

    [[nodiscard]] llvm::LLVMContext& context() noexcept;

    [[nodiscard]] llvm::IRBuilder<>* builder() noexcept;

    [[nodiscard]] llvm::Module* module() noexcept;

    [[nodiscard]] const llvm::DataLayout& layout() const noexcept;

    [[nodiscard]] std::unique_ptr<llvm::Module> take_module() noexcept;

  private:
    llvm::LLVMContext* context_;
    const llvm::TargetMachine& machine_;
    llvm::DataLayout layout_;
    std::unique_ptr<llvm::Module> module_;
    std::unique_ptr<llvm::IRBuilder<>> builder_;
  };
} // namespace gal::backend