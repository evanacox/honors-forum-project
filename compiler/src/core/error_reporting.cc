//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./error_reporting.h"
#include "../utility/flags.h"
#include "../utility/log.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include <string>

using namespace std::literals;

namespace {
  std::string header_colored(gal::DiagnosticType type, std::int64_t code) noexcept {
    auto builder = ""s;

    switch (type) {
      case gal::DiagnosticType::error:
        absl::StrAppend(&builder, gal::colors::code_bold_red, "error [E#", absl::Dec(code, absl::kZeroPad4), "]");
        break;
      case gal::DiagnosticType::warning:
        absl::StrAppend(&builder, gal::colors::code_bold_yellow, "warning [E#", absl::Dec(code, absl::kZeroPad4), "]");
        break;
      case gal::DiagnosticType::note:
        assert(code == -1);
        absl::StrAppend(&builder, gal::colors::bold_magenta("note"));
        break;
    }

    absl::StrAppend(&builder, gal::colors::bold_white(" :: "), gal::colors::code_reset);

    return builder;
  }

  std::string header_uncolored(gal::DiagnosticType type, std::int64_t code) noexcept {
    auto builder = ""s;

    switch (type) {
      case gal::DiagnosticType::error:
        absl::StrAppend(&builder, "error [E#", absl::Dec(code, absl::kZeroPad4), "]");
        break;
      case gal::DiagnosticType::warning:
        absl::StrAppend(&builder, "warning [E#", absl::Dec(code, absl::kZeroPad4), "]");
        break;
      case gal::DiagnosticType::note:
        assert(code == -1);
        absl::StrAppend(&builder, "note");
        break;
    }

    absl::StrAppend(&builder, " :: ");

    return builder;
  }

  std::string header(gal::DiagnosticType type, std::int64_t code) noexcept {
    if (gal::flags().colored()) {
      return header_colored(type, code);
    } else {
      return header_uncolored(type, code);
    }
  }
} // namespace

namespace gal {
  SingleMessage::SingleMessage(std::string message, DiagnosticType type, std::int64_t code) noexcept
      : message_{std::move(message)},
        type_{type},
        code_{code} {
    assert(code > -2);

    if (code > 0) {
      // code better be a valid one
      assert(gal::diagnostic_info(code).has_value());
    }
  }

  std::string SingleMessage::internal_build(std::string_view, std::string_view padding) const noexcept {
    auto message = gal::flags().colored() ? gal::colors::bold_white(message_) : message_;

    return absl::StrCat(padding, header(type_, code_), message);
  }

  UnderlineList::UnderlineList(std::vector<PointedOut> locs) noexcept : list_{std::move(locs)} {
    assert(!list_.empty());

    for (auto& loc : list_) {
      assert(loc.loc == list_.front().loc);
    }
  }

  std::string UnderlineList::internal_build(std::string_view, std::string_view) const noexcept {
    return ""s;
  }

  Diagnostic::Diagnostic(std::int64_t code, std::vector<std::unique_ptr<DiagnosticPart>> parts) noexcept
      : code_{code},
        parts_{std::move(parts)} {}

  std::string Diagnostic::build(std::string_view source) const noexcept {
    // if we somehow got here with an invalid code, we just need to crash
    auto info = gal::diagnostic_info(code_).value();

    // main message needs to show a code, the proper type, and the one liner
    auto main_message = SingleMessage(std::string{info.one_liner}, info.diagnostic_type, code_);

    // rest get joined. each doesn't end with a \n, so we want a \n between all of them
    auto rest = absl::StrJoin(parts_, "\n", [source](auto* out, auto& ptr) {
      out->append(ptr->build(source, "  "));
    });

    return absl::StrCat(main_message.build(source, ""), rest);
  }

  std::optional<DiagnosticInfo> diagnostic_info(std::int64_t code) noexcept {
    static absl::flat_hash_map<std::int64_t, DiagnosticInfo> lookup{
        {1,
            {"invalid builtin width",
                "integer builtin types must be of width 8/16/32/64/128, floats must have 32/64/128",
                gal::DiagnosticType::error}},
    };

    auto it = lookup.find(code);

    return (it == lookup.end()) //
               ? std::optional<DiagnosticInfo>(std::nullopt)
               : std::make_optional((*it).second);
  }
} // namespace gal
