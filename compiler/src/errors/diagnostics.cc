//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./diagnostics.h"
#include "../utility/flags.h"
#include "../utility/log.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/ascii.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include <string>

using namespace std::literals;

namespace ast = gal::ast;
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
#ifndef NDEBUG
    assert(!list_.empty());

    for (auto& loc : list_) {
      assert(loc.loc.file() == list_.front().loc.file());
    }
#endif

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

    important_loc_.emplace(it->loc);

    // sort so messages show up in the order they appear in the source
    std::sort(list_.begin(), list_.end(), [](PointedOut& lhs, PointedOut& rhs) {
      // if lines are equivalent, sort by column
      // else, sort by the line in ascending order
      return (lhs.loc.line() == rhs.loc.line()) ? (lhs.loc.column() < rhs.loc.column())
                                                : (lhs.loc.line() < rhs.loc.line());
    });
  }
} // namespace gal

namespace {
  struct UnderlineState {
    std::uint64_t max_line;
    std::string_view source;
    std::string_view padding;
    std::optional<std::uint64_t> previous_line;
  };

  std::pair<std::string, std::string> line_number_padding(std::uint64_t current, std::uint64_t max) noexcept {
    auto curr_str = gal::to_digits(current);
    auto max_str = gal::to_digits(max);

    return {std::string(max_str.size() - curr_str.size(), ' '), std::string(max_str.size(), ' ')};
  }

  void build_list(std::string* builder, const gal::UnderlineList::PointedOut& spot, UnderlineState* state) noexcept {
    auto& loc = spot.loc;
    auto full_line = break_into_lines(state->source)[loc.line() - 1];
    auto [before_line, without_line] = line_number_padding(loc.line(), state->max_line);
    auto [start, underlined, rest] = break_up(full_line, loc);
    auto underline = absl::StrCat(std::string(start.size(), ' '),
        diagnostic_color(spot.type, underline_with(underlined.size(), spot.underline)));

    // if there are any lines between the previous and current, add a ...
    if (state->previous_line != (loc.line() - 1) && state->previous_line.has_value()) {
      absl::StrAppend(builder, "\n", state->padding, without_line, "...\n");
    }

    absl::StrAppend(builder,
        state->padding,
        without_line, // `     | `
        " |\n",
        state->padding, // ` 123 |   if thing {`
        before_line,
        loc.line(),
        " | ",
        start,
        diagnostic_color(spot.type, underlined),
        rest,
        "\n",
        state->padding, // `     |      ~~~~~ blah blah blah explanation`
        without_line,
        " | ",
        underline,
        " ",
        diagnostic_color(spot.type, spot.message));

    state->previous_line = loc.line();
  }

  void append_file_info(std::string* builder, const UnderlineState& state, const ast::SourceLoc& loc) {
    absl::StrAppend(builder,
        state.padding,
        ">>> ",
        colors::code_blue,
        loc.file().string(),
        " (line ",
        loc.line(),
        ", column ",
        loc.column(),
        ")",
        colors::code_reset,
        "\n");
  }
} // namespace

namespace gal {
  std::string UnderlineList::internal_build(std::string_view source, std::string_view padding) const noexcept {
    auto builder = ""s;
    auto max_line = std::max_element(list_.begin(), list_.end(), [](auto& lhs, auto& rhs) {
      return lhs.loc.line() < rhs.loc.line();
    });

    auto state = UnderlineState{max_line->loc.line(), source, padding, std::nullopt};

    append_file_info(&builder, state, *important_loc_);

    for (auto& spot : list_) {
      build_list(&builder, spot, &state);
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
} // namespace gal

gal::DiagnosticInfo gal::diagnostic_info(std::int64_t code) noexcept {
  // while in theory this would be "more efficient" as an array
  // that uses `code + 1` as an index, here's the reasons why it doesn't work that way:
  //
  //   1. efficiency literally does not matter on the error path whatsoever,
  //      and even if it did, the 10-20 lookups into this table are not
  //      the "hot" part of even the error path.
  //
  //   2. if I have it like this, I can see the code number for whatever error
  //      I'm trying to find, without having to do literally anything
  //      additional. I also have the possibility of having gaps, although I
  //      am not taking advantage of that as of now
  //
  // it's just much easier on me to do it this way, and has basically zero impact on performance

  static absl::flat_hash_map<std::int64_t, gal::DiagnosticInfo> lookup{
      {1,
          {"invalid builtin width",
              "integer builtin types must be of width 8/16/32/64/128, floats must have 32/64/128",
              gal::DiagnosticType::error}},
      {2, {"invalid char literal", "char literal was unable to be parsed", gal::DiagnosticType::error}},
      {3, {"invalid integer literal", "integer literal was unable to be parsed", gal::DiagnosticType::error}},
      {4, {"invalid float literal", "float literal was unable to be parsed", gal::DiagnosticType::error}},
      {5, {"syntax error", "general syntax error in antlr4", gal::DiagnosticType::error}},
      {6,
          {"duplicate declaration name",
              "every declaration name must be unique in the module",
              gal::DiagnosticType::error}},
      {7,
          {"mismatched type for binding initializer",
              "if a binding has a type hint, the hint must match the real type of the initializer",
              gal::DiagnosticType::error}},
      {8,
          {"duplicate binding name",
              "every binding name must be unique in the same level of scope. shadowing is allowed in *different* "
              "levels of scope, but not the same",
              gal::DiagnosticType::error}},
      {9,
          {"conflicting function overloads",
              "overloads cannot have the same parameter types, or they would be ambiguous",
              gal::DiagnosticType::error}},
      {10,
          {"invalid type for struct-init expression",
              "the type of a struct-init expr must be a user-defined type, and not a `dyn` type",
              gal::DiagnosticType::error}},
      {10,
          {"invalid user-defined type",
              "a type must be the name given to a `type`, `struct` or `class` declaration, not any type of "
              "declaration",
              gal::DiagnosticType::error}},
      {11, {"unknown identifier name", "name did not resolve to a declaration", gal::DiagnosticType::error}},
      {12,
          {"missing initializer for struct field",
              "a struct-init expression must initialize every field of a struct",
              gal::DiagnosticType::error}},
      {13,
          {"mismatched types for struct field",
              "a struct initializer must evaluate to the same type as the associated struct field",
              gal::DiagnosticType::error}},
      {14, {"unknown type name", "name did not resolve to a type", gal::DiagnosticType::error}},
      {15, {"expected `bool` type for condition", "the condition must be of type `bool`", gal::DiagnosticType::error}},
      {16,
          {"mismatched types in if-expr",
              "all branches must evaluate to the same type in an if-expr",
              gal::DiagnosticType::error}},
      {17, {"invalid safe cast", "cannot perform a safe cast between these types", gal::DiagnosticType::error}},
      {18,
          {"unknown identifier",
              "variables must be declared before they can be used, does your variable exist?",
              gal::DiagnosticType::error}},
      {19,
          {"ambiguous reference to function",
              "you cannot reference or take the address of an overloaded function, you can only call it",
              gal::DiagnosticType::error}},
      {20,
          {"mismatched return type",
              "return expressions must return a type compatible with the function",
              gal::DiagnosticType::error}},
      {21,
          {"binding cannot be nil",
              "a binding without a type hint cannot be nil, it must be cast to a pointer type",
              gal::DiagnosticType::error}},
      {22,
          {"reference to declaration other than constant/function in identifier expression",
              "you can only reference constant declarations and function declarations in an id-expr, not all "
              "declarations",
              gal::DiagnosticType::error}},
      {22,
          {"reference to declaration other than constant/function in identifier expression",
              "you can only reference constant declarations and function declarations in an id-expr, not all "
              "declarations",
              gal::DiagnosticType::error}},
      {23,
          {"mismatched argument type in call expr",
              "each argument in a call must match the function type being called",
              gal::DiagnosticType::error}},
  };

  return lookup.at(code);
}

[[nodiscard]] std::unique_ptr<gal::DiagnosticPart> gal::point_out(const ast::Node& node,
    gal::DiagnosticType type,
    std::string inline_message) noexcept {
  return point_out(node.loc(), type, std::move(inline_message));
}

[[nodiscard]] std::unique_ptr<gal::DiagnosticPart> gal::point_out(const ast::SourceLoc& loc,
    gal::DiagnosticType type,
    std::string inline_message) noexcept {
  return std::make_unique<gal::UnderlineList>(std::vector{gal::point_out_part(loc, type, std::move(inline_message))});
}

gal::UnderlineList::PointedOut gal::point_out_part(const ast::Node& node,
    gal::DiagnosticType type,
    std::string inline_message) noexcept {
  return gal::point_out_part(node.loc(), type, std::move(inline_message));
}

gal::UnderlineList::PointedOut gal::point_out_part(const ast::SourceLoc& loc,
    gal::DiagnosticType type,
    std::string inline_message) noexcept {
  auto underline = (type == gal::DiagnosticType::note) ? gal::UnderlineType::straight : gal::UnderlineType::squiggly;

  return gal::UnderlineList::PointedOut{loc, std::move(inline_message), type, underline};
}

std::unique_ptr<gal::DiagnosticPart> gal::single_message(std::string message, gal::DiagnosticType type) noexcept {
  return std::make_unique<gal::SingleMessage>(std::move(message), type);
}