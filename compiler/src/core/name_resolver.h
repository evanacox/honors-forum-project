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

#include "../ast/modular_id.h"
#include "../ast/program.h"
#include "../errors/reporter.h"
#include "./environment.h"
#include <memory>
#include <string_view>

namespace gal {
  namespace internal {
    struct ModuleTable final {
      absl::flat_hash_map<std::string, std::unique_ptr<ModuleTable>> nested;
      gal::GlobalEnvironment env;
    };
  } // namespace internal

  /// Handles resolving symbol names for locals, functions, and
  /// any global/imported symbol names.
  ///
  /// Will modify the AST and replace nodes with qualified nodes
  /// where possible
  class NameResolver final {
  public:
    /// Begins resolving the AST, any unambiguous (i.e not scoped) symbols
    /// are immediately resolved
    ///
    /// \param program The program to resolve symbols for
    /// \param diagnostics A place to put diagnostics for the program
    explicit NameResolver(ast::Program* program, DiagnosticReporter* diagnostics) noexcept;

    /// Checks if there's an overload set going by the name `id`
    ///
    /// \param id The id to look up
    /// \return An overload set, if one exists
    [[nodiscard]] std::optional<const OverloadSet*> overloads(const ast::FullyQualifiedID& id) const noexcept;

    /// Gets an entity from `id`
    ///
    /// \param id The id to look up
    /// \return An entity, if one exists
    [[nodiscard]] std::optional<const GlobalEntity*> entity(const ast::FullyQualifiedID& id) const noexcept;

    /// Gets an entity from a fully-qualified ID that is specifically a type of some sort
    /// If it's a type alias, gets the type being aliased. If it's a struct/dyn trait type,
    /// it returns the correct type node for that
    ///
    /// \param id The id to look up
    /// \return A type node
    [[nodiscard]] std::optional<const ast::Type*> type(const ast::FullyQualifiedID& id) const noexcept;

    /// Gets the struct declaration associated with `id`, if it exists
    ///
    /// \param id The id to look up
    /// \return A possible struct declaration
    [[nodiscard]] std::optional<const ast::StructDeclaration*> struct_type(
        const ast::FullyQualifiedID& id) const noexcept;

    /// Gets the constant declaration associated with `id`, if it exists
    ///
    /// \param id The id to look up
    /// \return A possible constant declaration
    [[nodiscard]] std::optional<const ast::ConstantDeclaration*> constant(
        const ast::FullyQualifiedID& id) const noexcept;

    /// "Enters" a new scope and makes that the scope to add variables to
    void enter_scope() noexcept;

    /// Leaves the current scope, makes the current scope the scope that was entered before
    void leave_scope() noexcept;

    /// Checks if the entire scope/local environment contains a symbol called `name`.
    ///
    /// \param name The name to look for
    /// \return Whether or not that symbol exists anywhere
    [[nodiscard]] bool contains_local(std::string_view name) const noexcept;

    /// Adds a symbol to the current scope
    ///
    /// \param name The name of the symbol
    /// \param data Data about the symbol
    void add_local(std::string_view name, ScopeEntity data) noexcept;

    /// Gets the nearest local variable with name `name`
    ///
    /// \param name The name to get
    /// \return A local, if it exists
    [[nodiscard]] std::optional<const ast::Type*> local(std::string_view name) const noexcept;

    /// Finds the environment that has `id` in it, and returns a fully-qualified id if possible
    ///
    /// \param id The ID to look for
    /// \return The environment that contains `id` and a fully-qualified version of `id`
    [[nodiscard]] std::optional<std::pair<ast::FullyQualifiedID, gal::GlobalEnvironment*>> qualified_for(
        const ast::UnqualifiedID& id) noexcept;

  private:
    ast::Program* program_;
    gal::Environment env_;
    internal::ModuleTable root_; // the global envs in the hashmap are still owned by the tree at `root_`
    absl::flat_hash_map<std::string, gal::GlobalEnvironment*> fully_qualified_;
  };
} // namespace gal
