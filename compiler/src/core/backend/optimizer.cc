//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./optimizer.h"
#include "../../utility/flags.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/Passes.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"

namespace {
  std::string_view pass_name(gal::OptLevel level) {
    switch (level) {
      case gal::OptLevel::none: return "default<O0>";
      case gal::OptLevel::some: return "default<O1>";
      case gal::OptLevel::small: return "default<Os>";
      case gal::OptLevel::fast: return "default<O3>";
      default: assert(false); break;
    }

    return "default<O0>";
  }
} // namespace

namespace gal {
  void backend::optimize(llvm::Module* module, llvm::TargetMachine* machine) noexcept {
    auto level = gal::flags().opt();
    auto lam = llvm::LoopAnalysisManager{};
    auto fam = llvm::FunctionAnalysisManager{};
    auto cgam = llvm::CGSCCAnalysisManager{};
    auto mam = llvm::ModuleAnalysisManager{};
    auto builder = llvm::PassBuilder(machine);

    builder.registerModuleAnalyses(mam);
    builder.registerCGSCCAnalyses(cgam);
    builder.registerFunctionAnalyses(fam);
    builder.registerLoopAnalyses(lam);
    builder.crossRegisterProxies(lam, fam, cgam, mam);

    if (level != OptLevel::none) {
      fam.registerPass([&] {
        return builder.buildDefaultAAPipeline();
      });
    }

    auto mpm = llvm::ModulePassManager{};

    mpm.addPass(llvm::VerifierPass{});
    (void)builder.parsePassPipeline(mpm, pass_name(level));

    mpm.run(*module, mam);
  }
} // namespace gal