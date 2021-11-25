//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#pragma once

#include "../ast/nodes/node.h"
#include "../ast/source_loc.h"
#include <cassert>
#include <cstdint>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

namespace gal {
  /// Represents some sort of reportable diagnostic that the compiler needs
  /// to tell the user about, used to form parts of a full diagnostic
  class DiagnosticPart {
  public:
    [[nodiscard]] std::string build(std::string_view source, std::string_view padding = "") const noexcept {
      return internal_build(source, padding);
    }

    virtual ~DiagnosticPart() = default;

  protected:
    /// Builds a string that's ready-to-print
    ///
    /// \param source The source code of the entire program
    /// \return The message to be displayed to a user
    [[nodiscard]] virtual std::string internal_build(std::string_view source,
        std::string_view padding) const noexcept = 0;
  };

  enum class DiagnosticType {
    error,
    warning,
    note,
  };

  enum class UnderlineType {
    squiggly,       // ~~~~
    straight,       // ----
    carets,         // ^^^^
    straight_arrow, // ^---
    squiggly_arrow, // ^~~~
  };

  /// Type that gives a diagnostic a way to incorporate a message,
  /// also makes all diagnostics have a consistent style
  class SingleMessage final : public DiagnosticPart {
  public:
    /// Initializes the diagnostic mixin part of the type
    ///
    /// \param message The message to display
    /// \param type The type of diagnostic, e.g error or warning
    /// \param code A code to display along with the message if desired. Must not be negative or 0 if it is provided
    explicit SingleMessage(std::string message, DiagnosticType type, std::int64_t code = -1) noexcept;

  protected:
    [[nodiscard]] std::string internal_build(std::string_view, std::string_view padding) const noexcept final;

  private:
    std::string message_;
    DiagnosticType type_;
    std::int64_t code_ = -1;
  };

  /// Deals with **only** the underline/source code point-out
  /// part of a message. Correctly pretty-prints a set of
  /// underlines
  class UnderlineList final : public DiagnosticPart {
  public:
    struct PointedOut {
      ast::SourceLoc loc;
      std::string message = {};
      DiagnosticType type = DiagnosticType::error;
      UnderlineType underline = UnderlineType::squiggly;
    };

    /// Initializes the UnderlineList
    ///
    /// \param locs The spots in the source code to underline. Must all be in the same file, and must not be empty
    explicit UnderlineList(std::vector<PointedOut> locs) noexcept;

  protected:
    [[nodiscard]] std::string internal_build(std::string_view source, std::string_view padding) const noexcept final;

  private:
    std::vector<PointedOut> list_;
    std::optional<ast::SourceLoc> important_loc_;
  };

  /// A real diagnostic message that is ready to print
  class Diagnostic final {
  public:
    /// Initializes the diagnostic
    ///
    /// \param code The code of the warning/error
    /// \param parts Extra parts to the diagnostic, e.g source underlining
    explicit Diagnostic(std::int64_t code, std::vector<std::unique_ptr<DiagnosticPart>> parts) noexcept;

    /// Builds the diagnostic
    ///
    /// \param source The source code of the file the error comes from
    /// \return
    [[nodiscard]] std::string build(std::string_view source) const noexcept;

  private:
    std::int64_t code_;
    std::vector<std::unique_ptr<DiagnosticPart>> parts_;
  };

  /// Holds the key information about a diagnostic code
  /// that error reporting needs to be able to display
  struct DiagnosticInfo {
    /// A single-line short message explaining the diagnostic
    std::string_view one_liner;

    /// A longer-form explanation of the diagnostic, suitable for a note
    std::string_view explanation;

    /// The type of the diagnostic, i.e note/warning/error
    DiagnosticType diagnostic_type;
  };

  /// Gets the info, description and explanation for a diagnostic code
  ///
  /// \param code The code to look up
  /// \return The info for the code
  DiagnosticInfo diagnostic_info(std::int64_t code) noexcept;

  /// Reports a diagnostic and puts it to stdout
  ///
  /// \param source The source code to base errors off of
  /// \param diagnostic The diagnostic to show
  void report_diagnostic(std::string_view source, const Diagnostic& diagnostic) noexcept;

  /// Points out a bit of source code
  ///
  /// \param node The node to point out
  /// \param type The diagnostic type of the underline
  /// \param inline_message A message to show next to the underline
  /// \return A underline diagnostic part
  [[nodiscard]] std::unique_ptr<gal::DiagnosticPart> point_out(const ast::Node& node,
      gal::DiagnosticType type,
      std::string inline_message = {}) noexcept;

  /// Points out a specific source location
  ///
  /// \param loc The location to point out
  /// \param type The diagnostic type of the underline
  /// \param inline_message A message to show next to the underline
  /// \return A underline diagnostic part
  [[nodiscard]] std::unique_ptr<gal::DiagnosticPart> point_out(const ast::SourceLoc& loc,
      gal::DiagnosticType type,
      std::string inline_message = {}) noexcept;

  /// Points out a bit of source code
  ///
  /// \param node The node to point out
  /// \param type The type of point-out for it to be
  /// \param inline_message A message to put with the underline
  /// \return A point-out
  [[nodiscard]] gal::UnderlineList::PointedOut point_out_part(const ast::Node& node,
      gal::DiagnosticType type,
      std::string inline_message = {}) noexcept;

  /// Points out a bit of source code
  ///
  /// \param loc The location to point out
  /// \param type The type of point-out for it to be
  /// \param inline_message A message to put with the underline
  /// \return A point-out
  [[nodiscard]] gal::UnderlineList::PointedOut point_out_part(const ast::SourceLoc& loc,
      gal::DiagnosticType type,
      std::string inline_message = {}) noexcept;

  ///  Creates an UnderlineList from a list of `PointedOut`s
  ///
  /// \param args The list of `PointedOut`s
  /// \return An `UnderlineList`
  template <typename... Args> std::unique_ptr<gal::DiagnosticPart> point_out_list(Args&&... args) noexcept {
    auto list = gal::into_list(std::forward<Args>(args)...);

    return std::make_unique<gal::UnderlineList>(std::move(list));
  }
} // namespace gal
