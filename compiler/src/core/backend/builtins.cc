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
#include "llvm/ADT/ArrayRef.h"
#include "llvm/IR/InlineAsm.h"

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

      fn->setCallingConv(llvm::CallingConv::C);
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

        fn->setCallingConv(llvm::CallingConv::C);
        fn->setDoesNotReturn();
        fn->setDoesNotThrow();
        fn->addFnAttr(llvm::Attribute::Cold);
      }
    }

    void generate_puts(backend::LLVMState* state) noexcept {
      auto* module = state->module();
      auto* msg_type = llvm::Type::getInt8PtrTy(state->context());
      auto* size_type = llvm::Type::getIntNTy(state->context(), state->layout().getPointerSizeInBits());
      auto* type = llvm::FunctionType::get(llvm::Type::getVoidTy(state->context()), {msg_type, size_type}, false);
      auto* fn = llvm::Function::Create(type, llvm::Function::ExternalLinkage, llvm::Twine("__gallium_puts"), module);

      // can't be ArgMemOnly, since this may modify global i/o state
      fn->setCallingConv(llvm::CallingConv::C);
      fn->setDoesNotThrow();
      fn->addFnAttr(llvm::Attribute::WillReturn);
    }
  } // namespace

  void backend::generate_builtins(backend::LLVMState* state) noexcept {
    generate_builtin_trap(state);
    generate_panic_assert(state);
    generate_puts(state);
  }

  namespace {
    llvm::Value* call_trap(backend::LLVMState* state) noexcept {
      auto* fn = state->module()->getFunction("__gallium_trap");

      return state->builder()->CreateCall(fn);
    }

    llvm::Value* call_puts(backend::LLVMState* state, llvm::ArrayRef<llvm::Value*> args) noexcept {
      auto* builder = state->builder();
      assert(args.size() == 1);

      auto* fn = state->module()->getFunction("__gallium_puts");
      auto* ptr = builder->CreateExtractValue(args.front(), 0);
      auto* size = builder->CreateExtractValue(args.front(), 1);

      return builder->CreateCall(fn, {ptr, size});
    }

    llvm::Value* call_black_box(backend::LLVMState* state, llvm::ArrayRef<llvm::Value*> args) noexcept {
      auto* type = llvm::FunctionType::get(llvm::Type::getVoidTy(state->context()),
          {llvm::Type::getInt8PtrTy(state->context())},
          false);
      auto* inline_asm = llvm::InlineAsm::get(type, "", "r|m,~{memory},~{dirflag},~{fpsr},~{flags}", true);

      return state->builder()->CreateCall(inline_asm, args);
    }
  } // namespace

  llvm::Value* backend::call_builtin(std::string_view name,
      LLVMState* state,
      llvm::ArrayRef<llvm::Value*> args) noexcept {
    if (name == "__builtin_trap") {
      return call_trap(state);
    } else if (name == "__builtin_puts") {
      return call_puts(state, args);
    } else if (name == "__builtin_black_box") {
      return call_black_box(state, args);
    } else {
      assert(false);
    }

    return nullptr;
  }
} // namespace gal