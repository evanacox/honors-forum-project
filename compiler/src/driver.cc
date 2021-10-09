//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "driver.h"
#include "./syntax/generated/GalliumBaseVisitor.h"
#include "./syntax/generated/GalliumLexer.h"
#include "./syntax/generated/GalliumParser.h"
#include "./utility/flags.h"
#include "./utility/log.h"
#include "antlr4-runtime.h"

namespace gal {
  Driver::Driver() noexcept = default;

  int Driver::start(absl::Span<std::string_view> files) noexcept {
    std::istringstream ss(std::string{files[0]});
    antlr4::ANTLRInputStream is(ss);
    GalliumLexer lex(&is);
    antlr4::CommonTokenStream tokens(&lex);
    tokens.fill();

    if (gal::flags().verbose()) {
      for (auto token : tokens.getTokens()) {
        gal::outs() << token->toString() << "is whitespace: " << (token->getType() == GalliumLexer::WHITESPACE);
      }
    }

    GalliumParser parser(&tokens);
    auto* tree = parser.type();

    gal::outs() << tree->toStringTree(&parser, true);

    return 0;
  }
} // namespace gal