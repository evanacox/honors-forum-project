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

#include "./declaration_visitor.h"
#include "./expression.h"
#include "./modular_id.h"
#include "./statements.h"
#include "./type.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gal::ast {
  enum class DeclType {
    import_decl,
    import_from_decl,
    fn_decl,
    struct_decl,
    class_decl,
    type_decl,
  };

  /// Abstract base type for all "declaration" AST nodes
  ///
  /// Is able to be visited by a `DeclarationVisitorBase`, and can be queried
  /// on whether it's exported and what real type of declaration it is
  class Declaration {
  public:
    Declaration() = delete;

    /// Checks if the declaration is being `export`ed
    ///
    /// \return Whether or not the declaration is exported
    [[nodiscard]] bool exported() const noexcept {
      return exported_;
    }

    /// Gets the real declaration type that a declaration actually is
    ///
    /// \return The type of the declaration
    [[nodiscard]] DeclType type() const noexcept {
      return real_;
    }

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void accept(DeclarationVisitorBase* visitor) = 0;

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void accept(const ConstDeclarationVisitorBase& visitor) const = 0;

    /// Helper that allows a visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(DeclarationVisitor<T>* visitor) {
      accept(static_cast<DeclarationVisitorBase*>(visitor));

      return visitor->take_result();
    }

    /// Helper that allows a const visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(const ConstDeclarationVisitor<T>& visitor) {
      accept(static_cast<const ConstDeclarationVisitorBase&>(visitor));

      return visitor->take_result();
    }

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Declaration() = default;

  protected:
    /// Initializes the state of the declaration base class
    ///
    /// \param exported Whether or not this particular declaration is marked `export`
    explicit Declaration(bool exported, DeclType real) noexcept : exported_{exported}, real_{real} {}

    /// Protected so only derived can copy
    Declaration(const Declaration&) = default;

    /// Protected so only derived can move
    Declaration(Declaration&&) = default;

  private:
    bool exported_;
    DeclType real_;
  };

  /// Models a plain `import` declaration of the form `import foo::bar`.
  ///
  /// Simply contains the module imported and nothing else.
  class ImportDeclaration : public Declaration {
  public:
    /// Constructs an `ImportDeclaration`
    ///
    /// \param exported Whether or not the import is `export`ed
    /// \param module_imported The module being imported
    explicit ImportDeclaration(bool exported, ModuleID module_imported) noexcept
        : Declaration(exported, DeclType::import_decl),
          mod_{std::move(module_imported)} {}

    /// Gets the module that is being imported
    ///
    /// \return The module being imported
    [[nodiscard]] const ModuleID& mod() const noexcept {
      return mod_;
    }

    /// Accepts a visitor and calls the correct method
    void accept(DeclarationVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a const visitor and calls the correct method
    void accept(const ConstDeclarationVisitorBase& visitor) const final {
      visitor.visit(*this);
    }

  private:
    ModuleID mod_;
  };

  /// Modules an import-from declaration, i.e `import log2, log10, ln from core::math`.
  ///
  /// Instead of storing both the module and the list of names imported, each
  /// name is fully qualified based on the module it was imported from
  /// to make it easier to deal with later
  class ImportFromDeclaration final : public Declaration {
  public:
    /// Creates an `ImportFromDeclaration`
    ///
    /// \param exported Whether or not the decl was exported
    /// \param entities Each entity that was imported
    explicit ImportFromDeclaration(bool exported, std::vector<FullyQualifiedID> entities) noexcept
        : Declaration(exported, DeclType::import_from_decl),
          entities_{std::move(entities)} {}

    /// Gets a list of the entities imported
    ///
    /// \return Every name that is imported by the declaration
    [[nodiscard]] absl::Span<const FullyQualifiedID> imported_entities() const noexcept {
      return entities_;
    }

    /// Accepts a visitor and calls the correct method
    void accept(DeclarationVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a const visitor and calls the correct method
    void accept(const ConstDeclarationVisitorBase& visitor) const final {
      visitor.visit(*this);
    }

  private:
    std::vector<FullyQualifiedID> entities_;
  };

  /// Models a full function declaration, prototype and body.
  class FnDeclaration final : public Declaration {
  public:
    /// Functions can be marked with attributes
    enum class FnAttributeType {
      builtin_pure,          // __pure
      builtin_throws,        // __throws
      builtin_always_inline, // __alwaysinline
      builtin_inline,        // __inline
      builtin_no_inline,     // __noinline
      builtin_malloc,        // __malloc
      builtin_hot,           // __hot
      builtin_cold,          // __cold
      builtin_arch,          // __arch("cpu_arch")
      builtin_noreturn,      // __noreturn
    };

    /// An attribute can have other arguments given to it
    struct FnAttribute {
      FnAttributeType type;
      std::vector<std::string> args;
    };

    /// An argument is a name: type pair
    struct Argument {
      std::string name;
      std::unique_ptr<Type> type;
    };

    /// Creates a FnDeclaration
    ///
    /// \param exported Whether or not the function is exported
    /// \param external Whether or not the function is marked `extern`
    /// \param name The name of the function
    /// \param args The arguments to the function
    /// \param attributes Any attributes given for the function
    /// \param body The body of the function
    explicit FnDeclaration(bool exported,
        bool external,
        std::string name,
        std::vector<Argument> args,
        std::vector<FnAttribute> attributes,
        std::unique_ptr<BlockExpression> body) noexcept
        : Declaration(exported, DeclType::fn_decl),
          external_{external},
          name_{std::move(name)},
          args_{std::move(args)},
          attributes_{std::move(attributes)},
          body_{std::move(body)} {}

    /// Returns if the function is marked `extern`
    ///
    /// \return Whether the function is meant to be visible on FFI boundaries
    [[nodiscard]] bool external() const noexcept {
      return external_;
    }

    /// Gets the (unmangled) name of the function
    ///
    /// \return The name of the function
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets a list of the function arguments
    ///
    /// \return The function arguments
    [[nodiscard]] absl::Span<const Argument> args() const noexcept {
      return args_;
    }

    /// Gets a mutable list of the function arguments
    ///
    /// \return The function arguments
    [[nodiscard]] absl::Span<Argument> mut_args() noexcept {
      return {args_.data(), args_.size()};
    }

    /// Gets a list of the function attributes
    ///
    /// \return All function attributes
    [[nodiscard]] absl::Span<const FnAttribute> attributes() const noexcept {
      return attributes_;
    }

    /// Gets a mutable list of the function attributes
    ///
    /// \return All function attributes
    [[nodiscard]] absl::Span<FnAttribute> mut_attributes() noexcept {
      return {attributes_.data(), attributes_.size()};
    }

    /// Gets the body of the function
    ///
    /// \return The function body
    [[nodiscard]] const BlockExpression& body() const noexcept {
      return *body_;
    }

    /// Gets a mutable ptr to the body of the function
    ///
    /// \return A mutable ptr to the function body
    [[nodiscard]] BlockExpression* mut_body() const noexcept {
      return body_.get();
    }

    /// Correctly release and update the body for `*this`
    ///
    /// \param body The new body to use
    std::unique_ptr<BlockExpression> exchange_body(std::unique_ptr<BlockExpression> body) noexcept {
      return std::exchange(body_, std::move(body));
    }

    /// Accepts a visitor and calls the correct method
    void accept(DeclarationVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a const visitor and calls the correct method
    void accept(const ConstDeclarationVisitorBase& visitor) const final {
      visitor.visit(*this);
    }

  private:
    bool external_;
    std::string name_;
    std::vector<Argument> args_;
    std::vector<FnAttribute> attributes_;
    std::unique_ptr<BlockExpression> body_;
  };
} // namespace gal::ast