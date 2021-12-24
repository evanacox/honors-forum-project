//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./builtins.h"
#include "./llvm_state.h"

namespace gal {
  namespace {
    void generate_builtin_trap(backend::LLVMState* state) noexcept {
      auto* builder = state->builder();
      auto* module = state->module();

      auto* trap = llvm::Intrinsic::getDeclaration(module, llvm::Intrinsic::trap);
      auto* type = llvm::FunctionType::get(llvm::Type::getVoidTy(state->context()), false);
      auto* fn = llvm::Function::Create(type, llvm::Function::WeakODRLinkage, "__gallium_trap", module);
      auto* entry = llvm::BasicBlock::Create(state->context(), "entry", fn);
      builder->SetInsertPoint(entry);
      builder->CreateCall(trap);
      builder->CreateUnreachable();
      fn->addFnAttr(llvm::Attribute::AlwaysInline);
      fn->setDoesNotAccessMemory();
      fn->setDoesNotThrow();
      fn->setDoesNotRecurse();
      fn->setDoesNotReturn();
    }

    void generate_panic_assert(backend::LLVMState* state) noexcept {
      auto* module = state->module();
      auto* msg_type = llvm::Type::getInt8PtrTy(state->context());
      auto* line_type = llvm::Type::getInt64Ty(state->context());

      auto* type = // fn (*const u8, u64, *const u8) __noreturn -> void
          llvm::FunctionType::get(llvm::Type::getVoidTy(state->context()), {msg_type, line_type, msg_type}, false);

      for (auto name : {std::string_view{"__gallium_assert_fail"}, std::string_view{"__gallium_panic"}}) {
        auto* fn = llvm::Function::Create(type, llvm::Function::ExternalLinkage, llvm::Twine(name), module);

        fn->setDoesNotReturn();
        fn->setDoesNotThrow();
        fn->addFnAttr(llvm::Attribute::Cold);
      }
    }
  } // namespace

  void backend::generate_builtins(backend::LLVMState* state) noexcept {
    generate_builtin_trap(state);
    generate_panic_assert(state);
  }
} // namespace gal