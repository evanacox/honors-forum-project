//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./emit.h"
#include "../utility/flags.h"
#include "../utility/log.h"
#include "absl/strings/str_cat.h"
#include "llvm/Bitcode/BitcodeWriter.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Target/TargetMachine.h"
#include <cstdlib>
#include <string>
#include <utility>

namespace {
  std::string filename() {
    auto name = gal::flags().out();

    switch (gal::flags().emit()) {
      case gal::OutputFormat::llvm_ir: return absl::StrCat(name, ".ll");
      case gal::OutputFormat::llvm_bc: return absl::StrCat(name, ".bc");
      case gal::OutputFormat::assembly: return absl::StrCat(name, ".S");
      case gal::OutputFormat::object_code: return absl::StrCat(name, ".o");
      case gal::OutputFormat::static_lib: {
#ifdef _WIN64
        return absl::StrCat(name, ".lib");
#else
        return absl::StrCat(name, ".a");
#endif
      }
      case gal::OutputFormat::exe: {
#ifdef _WIN64
        return absl::StrCat(name, ".exe");
#else
        return std::string{name};
#endif
      }
      case gal::OutputFormat::ast_graphviz: return absl::StrCat(name, ".dot");
      default: assert(false); return "";
    }
  }

  void compile_with_system(std::string_view path) noexcept {
    auto compiler = std::string{std::getenv("CC")};
    auto command = absl::StrCat(compiler, " ", path, " -o ", path);

    std::system(command.c_str());
  }
} // namespace

bool gal::emit(llvm::Module* module, llvm::TargetMachine* machine) noexcept {
  auto ec = std::error_code{};
  auto file = filename();
  auto fd = llvm::raw_fd_ostream(file, ec, llvm::sys::fs::OF_None);

  if (ec) {
    gal::errs() << "unable to open file '" << file << "' for writing";

    return false;
  }

  auto emit_type = llvm::CGFT_Null;

  switch (gal::flags().emit()) {
    case OutputFormat::llvm_ir: module->print(fd, nullptr); return true;
    case OutputFormat::llvm_bc: llvm::WriteBitcodeToFile(*module, fd); return true;
    case OutputFormat::assembly: {
      emit_type = llvm::CGFT_AssemblyFile;
      break;
    }
    case OutputFormat::object_code:
    case OutputFormat::static_lib:
    case OutputFormat::exe: emit_type = llvm::CGFT_ObjectFile; break;
    case OutputFormat::ast_graphviz: assert(false); break;
  }

  // I don't know a better way to do this for any target, and I also can't seem to
  // find a way to hook this into the earlier pass manager
  auto emitter = llvm::legacy::PassManager{};

  if (machine->addPassesToEmitFile(emitter, fd, nullptr, emit_type)) {
    gal::errs() << "LLVM is unable to emit a file of the type requested!";

    return false;
  }

  emitter.run(*module);
  fd.flush();

  if (gal::flags().emit() == OutputFormat::exe) {
    compile_with_system(file);
  }

  return true;
}
