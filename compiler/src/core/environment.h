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

#include "../ast/nodes/declaration.h"
#include "../ast/nodes/type.h"
#include "../ast/program.h"
#include "../errors/reporter.h"
#include "absl/cleanup/cleanup.h"
#include "absl/container/btree_map.h"
#include "absl/container/flat_hash_map.h"
#include <tuple>
#include <vector>

namespace gal {
  /// References a global entity that's actually able to be accessed by name
  class GlobalEntity final {
  public:
    /// Creates a GlobalEntity
    ///
    /// \param name The name of the entity
    /// \param decl A pointer to the declaration
    explicit GlobalEntity(std::string_view name, ast::Declaration* decl) noexcept : name_{name}, decl_{decl} {}

    /// Gets the name of the declaration
    ///
    /// \return The name
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets the declaration
    ///
    /// \return The declaration
    [[nodiscard]] const ast::Declaration& decl() const noexcept {
      return *decl_;
    }

    /// Gets the declaration
    ///
    /// \return The declaration
    [[nodiscard]] ast::Declaration* decl_mut() noexcept {
      return decl_;
    }

    [[nodiscard]] std::optional<const ast::Type*> type() const noexcept {
      return (type_ != nullptr) ? std::make_optional(type_.get()) : std::nullopt;
    }

    [[nodiscard]] std::optional<ast::Type*> type_mut() noexcept {
      return (type_ != nullptr) ? std::make_optional(type_.get()) : std::nullopt;
    }

    [[nodiscard]] std::unique_ptr<ast::Type>* type_owner() noexcept {
      return &type_;
    }

  private:
    std::string_view name_;           // simply references the name from the entity's AST node
    ast::Declaration* decl_;          // same here, reference the entity's AST node
    std::unique_ptr<ast::Type> type_; // a type, if its needed
  };

  /// Represents part of an overload set
  class Overload {
  public:
    /// Creates an overload
    ///
    /// \param decl The function that is an overload
    explicit Overload(ast::FnDeclaration* decl) noexcept : decls_{decl} {}

    /// Creates an overload
    ///
    /// \param decl The function that is an overload
    explicit Overload(ast::ExternalFnDeclaration* decl) noexcept : decls_{decl} {}

    /// Gets a const pointer to the declaration, with type info preserved
    ///
    /// \return A const pointer to the decl
    [[nodiscard]] std::variant<const ast::FnDeclaration*, const ast::ExternalFnDeclaration*> decl() const noexcept {
      if (auto* const* ptr = std::get_if<ast::FnDeclaration*>(&decls_)) {
        return static_cast<const ast::FnDeclaration*>(*ptr);
      }

      auto* const* ptr = std::get_if<ast::ExternalFnDeclaration*>(&decls_);
      assert(ptr != nullptr);

      return static_cast<const ast::ExternalFnDeclaration*>(*ptr);
    }

    /// Gets a  pointer to the declaration, with type info preserved
    ///
    /// \return A pointer to the decl
    [[nodiscard]] std::variant<ast::FnDeclaration*, ast::ExternalFnDeclaration*> decl_mut() noexcept {
      return decls_;
    }

    /// Casts the pointer to the base, and returns it
    ///
    /// \return The base pointer
    [[nodiscard]] const ast::Declaration& decl_base() const noexcept {
      return std::visit(
          [](auto* ptr) -> decltype(auto) {
            return static_cast<const ast::Declaration&>(*ptr);
          },
          decls_);
    }

    /// Casts the pointer to the base, and returns it
    ///
    /// \return The base pointer
    [[nodiscard]] ast::Declaration* decl_base_mut() noexcept {
      return std::visit(
          [](auto* ptr) {
            return static_cast<ast::Declaration*>(ptr);
          },
          decls_);
    }

    /// Gets the prototype of the function regardless of what
    /// type of function it is
    ///
    /// \return The prototype
    [[nodiscard]] const ast::FnPrototype& proto() const noexcept {
      return *std::visit(
          [](auto* ptr) {
            return ptr->proto_mut();
          },
          decls_);
    }

    /// Gets the location of the overload, regardless of type
    ///
    /// \return The type of the fn
    [[nodiscard]] const ast::SourceLoc& loc() const noexcept {
      return decl_base().loc();
    }

  private:
    std::variant<ast::FnDeclaration*, ast::ExternalFnDeclaration*> decls_;
  };

  /// Models a set of function overloads
  class OverloadSet final {
  public:
    /// Creates an empty overload set
    ///
    /// \param name The name of the set
    explicit OverloadSet(std::string_view name) noexcept : name_{name} {}

    /// Gets the name that connects all the overloads in the set
    ///
    /// \return The name of the functions
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets the list of all possible overloads for `name`
    ///
    /// \return The list of overloads
    [[nodiscard]] absl::Span<const Overload> fns() const noexcept {
      return functions_;
    }

    /// Gets the list of all possible overloads for `name`
    ///
    /// \return The list of overloads
    [[nodiscard]] absl::Span<Overload> fns() noexcept {
      return absl::MakeSpan(functions_);
    }

    /// Adds an overload to the overload set
    ///
    /// \param overload The overload to add
    void add_overload(Overload overload) noexcept;

  private:
    std::string_view name_;
    std::vector<Overload> functions_;
  };

  /// Models the "global environment" for a module, i.e a single file
  class GlobalEnvironment final {
  public:
    /// Correctly finds and creates the "global" environment based on
    /// a single AST
    ///
    /// \param program The AST to get the global environment from
    explicit GlobalEnvironment(ast::Program* program, gal::DiagnosticReporter* reporter) noexcept;

    /// Checks if `name` is in this environment at all, i.e whether any
    /// of the categories of entities contains it
    ///
    /// \param name The name to search for
    /// \return Whether there is an entity/overload/anything going by `name`
    [[nodiscard]] bool contains_any(std::string_view name) const noexcept;

    /// Gets a unique entity, i.e not a function
    ///
    /// \param name The name to try to get
    /// \return An entity with that name, if it exists
    [[nodiscard]] std::optional<GlobalEntity*> entity(std::string_view name) noexcept;

    /// Gets a unique entity, i.e not a function
    ///
    /// \param name The name to try to get
    /// \return An entity with that name, if it exists
    [[nodiscard]] std::optional<const GlobalEntity*> entity(std::string_view name) const noexcept;

    /// Gets an overload set of the name `name`, if it exists
    ///
    /// \param name The name to try to get
    /// \return An overload set with that name, if it exists
    [[nodiscard]] std::optional<OverloadSet*> overloads(std::string_view name) noexcept;

    /// Gets an overload set of the name `name`, if it exists
    ///
    /// \param name The name to try to get
    /// \return An overload set with that name, if it exists
    [[nodiscard]] std::optional<const OverloadSet*> overloads(std::string_view name) const noexcept;

  private:
    friend class NameResolver;

    absl::flat_hash_map<std::string, GlobalEntity> entities_;
    absl::flat_hash_map<std::string, OverloadSet> overloads_;
  };

  /// Represents an entity in the symbol table
  class ScopeEntity final {
  public:
    /// Creates a ScopeEntity
    ///
    /// \param loc The location in the source of it
    /// \param ptr A pointer to the entity's type
    explicit ScopeEntity(ast::SourceLoc loc, ast::Type* ptr, bool mut) noexcept
        : loc_{std::move(loc)},
          type_{ptr},
          mut_{mut} {}

    /// Gets the location where the entity was declared
    ///
    /// \return The location
    [[nodiscard]] const ast::SourceLoc& loc() const noexcept {
      return loc_;
    }

    /// Checks if the entity is mutable or not
    ///
    /// \return Whether or not it's mutable
    [[nodiscard]] bool mut() const noexcept {
      return mut_;
    }

    /// Gets the type of the entity
    ///
    /// \return The type
    [[nodiscard]] const ast::Type& type() const noexcept {
      return *type_;
    }

    /// Gets the type of the entity
    ///
    /// \return The type
    [[nodiscard]] ast::Type* type_mut() noexcept {
      return type_;
    }

  private:
    ast::SourceLoc loc_;
    ast::Type* type_;
    bool mut_;
  };

  /// Represents a single level of scope
  class Scope final {
  public:
    /// Gets the entity referred to by a symbol, if it exists in this scope
    ///
    /// \param name The name to get
    /// \return An entity, if the symbol exists in this scope
    [[nodiscard]] std::optional<const ScopeEntity*> get(std::string_view name) const noexcept;

    /// Checks if a variable name exists in the vector
    ///
    /// \param name The name to look for
    /// \return Whether or not the variable exists
    [[nodiscard]] bool contains(std::string_view name) const noexcept;

    /// Add a symbol to the scope
    ///
    /// \param name The name of the symbol
    /// \param data The data needed to add a variable
    /// \param diagnostics A diagnostics vector to add to if needed
    /// \return Returns whether or not it was added successfully
    bool add(std::string_view name, ScopeEntity data, gal::DiagnosticReporter* diagnostics) noexcept;

  private:
    absl::flat_hash_map<std::string, ScopeEntity> variables_;
  };

  /// Models the entire symbol "environment" for a program
  ///
  /// Holds variables, and the globally scoped variables/functions
  class Environment final {
  public:
    /// Creates an environment
    ///
    /// \param reporter The diagnostic reporter to use
    explicit Environment(gal::DiagnosticReporter* reporter) noexcept;

    /// Scans through all scopes, and gets the entity referred to by a name if it exists
    ///
    /// \param name The name to look for
    /// \return A pointer to the entity, if it exists
    [[nodiscard]] std::optional<const ScopeEntity*> get(std::string_view name) const noexcept;

    /// Checks if the entire environment contains a symbol called `name`.
    ///
    /// \param name The name to look for
    /// \return Whether or not that symbol exists anywhere
    [[nodiscard]] bool contains(std::string_view name) const noexcept;

    /// Adds a symbol to the current scope
    ///
    /// \param name The name of the symbol
    /// \param data Data about the symbol
    void add(std::string_view name, ScopeEntity data) noexcept;

    /// Pushes a scope into the environment
    ///
    /// \param scope The scope to add
    void push(Scope scope) noexcept;

    /// Removes the last scope from the environment
    void pop() noexcept;

    /// "Enters" a new scope and makes that the scope to add variables to
    void enter_scope() noexcept;

    /// Leaves the current scope, makes the current scope the scope that was entered before
    void leave_scope() noexcept;

    /// Gets every scope in the entire environment of the program at this instant
    ///
    /// \return The scopes in the AST at this point
    [[nodiscard]] absl::Span<const Scope> scopes() const noexcept;

  private:
    std::vector<Scope> scopes_;
    gal::DiagnosticReporter* diagnostics_;
  };
} // namespace gal
