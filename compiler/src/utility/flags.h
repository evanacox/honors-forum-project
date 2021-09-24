//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include <map>
#include <string>
#include <variant>
#include <vector>

namespace gallium {
  using ArgValue = std::variant<std::monostate, //
      std::string,
      long long,
      bool,
      std::vector<std::variant<std::string, long long, bool>>>;

  ///
  ///
  ///
  std::map<std::string, ArgValue> parse_arguments(int argc, char** argv) noexcept;

  ///
  ///
  ///
  class CliApp {};
} // namespace gallium