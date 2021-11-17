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
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include <string>

using namespace std::literals;

namespace colors = gal::colors;
namespace fs = std::filesystem;

namespace {
  absl::Span<const std::string_view> break_into_lines(std::string_view source) noexcept {
    // uses the **address** of the source location, since it should always be the same string
    static absl::flat_hash_map<const void*, std::vector<std::string_view>> line_lookup;

    if (auto it = line_lookup.find(source.data()); it != line_lookup.end()) {
      return absl::MakeConstSpan((*it).second);
    }

    auto lines = std::vector<std::string_view>{absl::StrSplit(source, absl::ByAnyChar("\r\n"))};

    std::transform(lines.begin(), lines.end(), lines.begin(), [](std::string_view s) {
      s = absl::StripPrefix(s, "\n");
      s = absl::StripPrefix(s, "\r\n");
      s = absl::StripSuffix(s, "\n");
      s = absl::StripSuffix(s, "\r\n");

      return s;
    });

    auto [it, _] = line_lookup.emplace(source.data(), std::move(lines));

    return absl::MakeConstSpan(it->second);
  }

  std::string header_colored(gal::DiagnosticType type, std::int64_t code) noexcept {
    auto builder = ""s;

    switch (type) {
      case gal::DiagnosticType::error:
        absl::StrAppend(&builder, gal::colors::code_bold_red, "error [E#", absl::Dec(code, absl::kZeroPad4), "] ");
        break;
      case gal::DiagnosticType::warning:
        absl::StrAppend(&builder, gal::colors::code_bold_yellow, "warning [E#", absl::Dec(code, absl::kZeroPad4), "] ");
        break;
      case gal::DiagnosticType::note:
        assert(code == -1);
        absl::StrAppend(&builder, gal::colors::bold_magenta("note "));
        break;
    }

    return builder;
  }

  std::string header_uncolored(gal::DiagnosticType type, std::int64_t code) noexcept {
    auto builder = ""s;

    switch (type) {
      case gal::DiagnosticType::error:
        absl::StrAppend(&builder, "error [E#", absl::Dec(code, absl::kZeroPad4), "] ");
        break;
      case gal::DiagnosticType::warning:
        absl::StrAppend(&builder, "warning [E#", absl::Dec(code, absl::kZeroPad4), "] ");
        break;
      case gal::DiagnosticType::note:
        assert(code == -1);
        absl::StrAppend(&builder, "note ");
        break;
    }

    return builder;
  }

  std::string header(gal::DiagnosticType type, std::int64_t code) noexcept {
    if (gal::flags().colored()) {
      return header_colored(type, code);
    } else {
      return header_uncolored(type, code);
    }
  }

  std::string diagnostic_color(gal::DiagnosticType type, std::string_view text) noexcept {
    switch (type) {
      case gal::DiagnosticType::error: return colors::red(text);
      case gal::DiagnosticType::note: return colors::magenta(text);
      case gal::DiagnosticType::warning: return colors::yellow(text);
      default: assert(false); return "";
    }
  }

  std::string underline_with(std::size_t length, gal::UnderlineType type) noexcept {
    switch (type) {
      case gal::UnderlineType::squiggly: return std::string(length, '~');
      case gal::UnderlineType::squiggly_arrow: return absl::StrCat("^", std::string(length - 1, '~'));
      case gal::UnderlineType::straight: return std::string(length, '-');
      case gal::UnderlineType::straight_arrow: return absl::StrCat("^", std::string(length, '~'));
      case gal::UnderlineType::carets: return std::string(length, '^');
      default: assert(false); return "";
    }
  }

  struct LineParts {
    std::string_view start, pointed_out, rest;
  };

  [[nodiscard]] LineParts break_up(std::string_view line, const gal::ast::SourceLoc& loc) noexcept {
    auto start = line.substr(0, loc.column() - 1);
    auto underlined = line.substr(loc.column() - 1, loc.length());
    auto rest = line.substr(loc.column() + loc.length() - 1);

    return {start, underlined, rest};
  }
} // namespace

namespace gal {
  SingleMessage::SingleMessage(std::string message, DiagnosticType type, std::int64_t code) noexcept
      : message_{std::move(message)},
        type_{type},
        code_{code} {
    assert(code > -2);
  }

  std::string SingleMessage::internal_build(std::string_view, std::string_view padding) const noexcept {
    auto message = gal::flags().colored() ? gal::colors::bold_white(message_) : message_;

    return absl::StrCat(padding, header(type_, code_), message);
  }

  UnderlineList::UnderlineList(std::vector<PointedOut> locs) noexcept : list_{std::move(locs)} {
    assert(!list_.empty());

    for (auto& loc : list_) {
      assert(loc.loc.file() == list_.front().loc.file());
    }

    // find first error if it exists
    auto it = std::find_if(list_.begin(), list_.end(), [](PointedOut& info) {
      return info.type == DiagnosticType::error;
    });

    // if it doesn't, find first warning
    if (it == list_.end()) {
      it = std::find_if(list_.begin(), list_.end(), [](PointedOut& info) {
        return info.type == DiagnosticType::warning;
      });
    }

    // if no warnings OR errors exist, just get iter to front
    if (it == list_.end()) {
      it = list_.begin();
    }

    std::swap(*it, list_.front());
  }

  std::string UnderlineList::internal_build(std::string_view source, std::string_view padding) const noexcept {
    auto builder = ""s;

    auto& main_loc = list_.front().loc;
    auto& relative_file = main_loc.file();
    absl::StrAppend(&builder,
        padding,
        ">>> ",
        colors::code_blue,
        relative_file.string(),
        ":",
        main_loc.line(),
        ":",
        main_loc.column(),
        colors::code_reset,
        "\n");

    for (auto& info : list_) {
      auto& loc = info.loc;
      auto lines = break_into_lines(source);
      auto full_line = lines[loc.line() - 1];
      auto line_padding = std::string(std::to_string(loc.line()).length(), ' ');
      auto [start, underlined, rest] = break_up(full_line, loc);
      auto underline = absl::StrCat(std::string(start.size(), ' '),
          diagnostic_color(info.type, underline_with(underlined.size(), info.underline)));

      absl::StrAppend(&builder,
          padding,
          line_padding,
          " |\n",
          padding,
          loc.line(),
          " | ",
          start,
          diagnostic_color(info.type, underlined),
          rest,
          "\n",
          padding,
          line_padding,
          " | ",
          underline);
    }

    return builder;
  }

  Diagnostic::Diagnostic(std::int64_t code, std::vector<std::unique_ptr<DiagnosticPart>> parts) noexcept
      : code_{code},
        parts_{std::move(parts)} {
    auto info = gal::diagnostic_info(code_);

    parts_.push_back(std::make_unique<SingleMessage>(std::string{info.explanation}, gal::DiagnosticType::note));
  }

  std::string Diagnostic::build(std::string_view source) const noexcept {
    auto info = gal::diagnostic_info(code_);

    // main message needs to show a code, the proper type, and the one liner
    auto main_message = SingleMessage(std::string{info.one_liner}, info.diagnostic_type, code_);

    // rest get joined. each doesn't end with a \n, so we want a \n between all of them
    auto rest = absl::StrJoin(parts_, "\n", [source](auto* out, auto& ptr) {
      out->append(ptr->build(source, " "));
    });

    return absl::StrCat(main_message.build(source, ""), "\n", rest);
  }

  DiagnosticInfo diagnostic_info(std::int64_t code) noexcept {
    static absl::flat_hash_map<std::int64_t, DiagnosticInfo> lookup{
        {1,
            {"invalid builtin width",
                "integer builtin types must be of width 8/16/32/64/128, floats must have 32/64/128",
                gal::DiagnosticType::error}},
        {2, {"invalid char literal", "char literal was unable to be parsed", gal::DiagnosticType::error}},
        {3, {"invalid integer literal", "integer literal was unable to be parsed", gal::DiagnosticType::error}},
        {4, {"invalid float literal", "float literal was unable to be parsed", gal::DiagnosticType::error}},
        {5, {"syntax error", "general syntax error in antlr4", gal::DiagnosticType::error}},
    };

    return lookup.at(code);
  }

  void report_diagnostic(std::string_view source, const Diagnostic& diagnostic) noexcept {
    gal::raw_errs() << diagnostic.build(source) << "\n\n";
  }
} // namespace gal
