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
#include "./errors/console_reporter.h"
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
} // namespace

namespace gal {
  int Driver::start(absl::Span<std::string_view> files) noexcept {
    for (auto& file : files) {
      auto path = fs::relative(file);
      auto data = read_file(fs::absolute(file));
      auto diagnostic = gal::ConsoleReporter(&gal::raw_outs(), data);

      if (auto program = parse_file(path, data, &diagnostic)) {
        auto valid = gal::type_check(*program, &diagnostic);

        if (gal::flags().verbose()) {
          gal::raw_outs() << gal::pretty_print(**program) << '\n';
        }

        gal::outs() << "is valid: " << valid;
      }
    }

    return 0;
  }

  std::optional<ast::Program*> Driver::parse_file(std::filesystem::path path,
      std::string_view source,
      gal::DiagnosticReporter* reporter) noexcept {
    if (auto result = gal::parse(std::move(path), source, reporter)) {
      programs_.push_back(std::move(*result));

      return &programs_.back();
    } else {
      return std::nullopt;
    }
  }
} // namespace gal