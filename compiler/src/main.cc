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
#include "utility/flags.h"
#include <string_view>
#include <vector>

#if defined(_WIN32) && defined(__MINGW32__) && defined(__GNUC__)
#include <windows.h>
#endif

namespace {
  std::vector<std::string_view> into_positionals(absl::Span<char*> positionals) noexcept {
    std::vector<std::string_view> result;
    result.reserve(positionals.size());

    for (auto* c_str : positionals) {
      // requires a strlen call unfortunately, but w/e
      result.emplace_back(c_str);
    }

    return result;
  }
} // namespace

int main(int argc, char** argv) {
#if defined(_WIN32) && defined(__MINGW32__) && defined(__GNUC__)
  // MingW defaults to UTF-8 string literals, this breaks pretty printer
  // and similar code due to UTF-8 code page not being the default on Windows
  SetConsoleOutputCP(CP_UTF8);
#endif

  absl::SetProgramUsageMessage(
      absl::StrCat("Invokes the Gallium compiler.\n\nSample Usage:\n\n    ", argv[0], " <file>"));

  auto vec = absl::ParseCommandLine(argc, argv);
  auto files = into_positionals(absl::MakeSpan(vec));

  gal::delegate_flags();

  // `files` includes the first positional argument (which is the exe path), need to ignore
  // also need to ignore the final null string, it will always have a string with just \0 in it
  return gal::Driver{}.start({files.data() + 1, files.size() - 1});
}