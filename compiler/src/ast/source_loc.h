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

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>

namespace gal::ast {
  /// Represents an exact location in the source code, showing
  /// exactly where an AST node came from.
  class SourceLoc {
  public:
    /// Creates a SourceLoc object
    ///
    /// \param raw The full raw text for a given node
    /// \param line The line that the node starts at
    /// \param column The column in that line the node starts at
    /// \param file The file that the node came from
    explicit SourceLoc(std::string raw, std::uint64_t line, std::uint64_t column, std::filesystem::path file) noexcept
        : raw_{std::move(raw)},
          line_{line},
          col_{column},
          file_{std::move(file)} {}

    /// The full raw text of the node
    ///
    /// \return Raw text
    [[nodiscard]] std::string_view raw_text() const noexcept {
      return raw_;
    }

    /// The line number of the node
    ///
    /// \return The line number
    [[nodiscard]] std::uint64_t line() const noexcept {
      return line_;
    }

    /// Gets the column that the node is on
    ///
    /// \return The column of the node
    [[nodiscard]] std::uint64_t column() const noexcept {
      return col_;
    }

    /// Gets the path of the file that the node was parsed from
    ///
    /// \return The file that the node came from
    [[nodiscard]] const std::filesystem::path& file() const noexcept {
      return file_;
    }

    /// Compares two source locations for equality
    ///
    /// \param other The other location to compare
    /// \return True if they are equivalent, false otherwise
    [[nodiscard]] bool operator==(const SourceLoc& other) const noexcept {
      return raw_text() == other.raw_text() && line() == other.line() && column() == other.column()
             && file() == other.file();
    }

  private:
    std::string raw_;
    std::uint64_t line_;
    std::uint64_t col_;
    std::filesystem::path file_;
  };
} // namespace gal::ast
