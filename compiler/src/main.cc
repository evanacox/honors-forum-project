//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "absl/flags/parse.h"
#include "absl/flags/usage.h"
#include "absl/strings/str_cat.h"
#include "driver.h"
#include <string_view>
#include <vector>

namespace {
  std::vector<std::string_view> into_positionals(absl::Span<char*> positionals) noexcept {
    std::vector<std::string_view> result;
    result.reserve(positionals.size());

    for (auto* ptr : positionals) {
      // requires a strlen call unfortunately
      result.emplace_back(ptr);
    }

    return result;
  }
} // namespace

int main(int argc, char** argv) {
  absl::SetProgramUsageMessage(
      absl::StrCat("Invokes the Gallium compiler.\n\nSample Usage:\n\n    ", argv[0], " <file>"));

  auto vec = absl::ParseCommandLine(argc, argv);
  auto files = into_positionals({vec.data(), vec.size()});

  return galc::Driver{}.start({files.data(), files.size()});
}