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
#include "./core/typechecker.h"
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
    file_data.resize(in.tellg(), ' ');
    in.seekg(0);
    in.read(file_data.data(), static_cast<std::streamsize>(file_data.size()));

    return file_data;
  }

  void report_errors(std::string_view file, absl::Span<const gal::Diagnostic> diagnostics) noexcept {
    for (auto& diagnostic : diagnostics) {
      gal::report_diagnostic(file, diagnostic);
    }
  }
} // namespace

namespace gal {
  int Driver::start(absl::Span<std::string_view> files) noexcept {
    for (auto& file : files) {
      if (auto program = parse_file(file)) {
        gal::raw_outs() << gal::pretty_print(**program) << '\n';
        gal::type_check(*program);
      }
    }

    return 0;
  }

  std::optional<ast::Program*> Driver::parse_file(std::string_view path) noexcept {
    auto data = read_file(fs::absolute(path));
    auto result = gal::parse(path, data);

    if (auto* program = std::get_if<ast::Program>(&result)) {
      programs_.push_back(std::move(*program));

      return &programs_.back();
    } else {
      report_errors(data, std::get<std::vector<gal::Diagnostic>>(result));

      return std::nullopt;
    }
  }
} // namespace gal