//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./driver.h"
#include "./syntax/parser.h"
#include "./utility/flags.h"
#include "./utility/log.h"
#include "./utility/pretty.h"
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using namespace std::literals;

namespace {
  std::string read_file(fs::path path) noexcept {
    auto in = std::ifstream(path);
    auto file_data = ""s;

    in.seekg(0, std::ios::end);
    file_data.resize(in.tellg());
    in.seekg(0);
    in.read(file_data.data(), static_cast<std::streamsize>(file_data.size()));

    return file_data;
  }
} // namespace

namespace gal {
  Driver::Driver() noexcept = default;

  int Driver::start(absl::Span<std::string_view> files) noexcept {
    for (auto& file : files) {
      auto data = read_file(fs::absolute(file));

      gal::outs() << std::quoted(data);

      auto result = gal::parse(data);

      gal::outs() << "was able to parse `" << file << "`: " << result.has_value();

      if (result.has_value()) {
        gal::outs() << gal::pretty_print(*result);
      }
    }

    return 0;
  }
} // namespace gal