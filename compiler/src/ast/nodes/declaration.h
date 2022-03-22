//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#pragma once

#include "../modular_id.h"
#include "../visitors/declaration_visitor.h"
#include "./ast_node.h"
#include "./expression.h"
#include "./statement.h"
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
    method_decl,
    external_decl,
    external_fn_decl,
    constant_decl,
    error_decl,
  };

  /// Abstract base type for all "declaration" AST nodes
  ///
  /// Is able to be visited by a `DeclarationVisitorBase`, and can be queried
  /// on whether it's exported and what real type of declaration it is
  class Declaration : public Node {
  public:
    Declaration() = delete;

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Declaration() = default;

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
    void accept(DeclarationVisitorBase* visitor) {
      internal_accept(visitor);
    }

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    void accept(ConstDeclarationVisitorBase* visitor) const {
      internal_accept(visitor);
    }

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
    template <typename T> T accept(ConstDeclarationVisitor<T>* visitor) const {
      accept(static_cast<ConstDeclarationVisitorBase*>(visitor));

      return visitor->take_result();
    }

    /// Compares two nodes for equality
    ///
    /// \param lhs The first node to compare
    /// \param rhs The second node to compare
    /// \return Whether the two are equal
    [[nodiscard]] friend bool operator==(const Declaration& lhs, const Declaration& rhs) noexcept {
      if (lhs.is(DeclType::error_decl) || rhs.is(DeclType::error_decl)) {
        return true;
      }

      return lhs.type() == rhs.type() && lhs.internal_equals(rhs);
    }

    /// Compares two nodes for inequality
    ///
    /// \param lhs The first node to compare
    /// \param rhs The second node to compare
    /// \return Whether the two are unequal
    [[nodiscard]] friend bool operator!=(const Declaration& lhs, const Declaration& rhs) noexcept {
      return !(lhs == rhs);
    }

    /// Compares two type nodes for complete equality, including source location.
    /// Equivalent to `a == b && a.loc() == b.loc()`
    ///
    /// \param rhs The other node to compare
    /// \return Whether the nodes are identical in every observable way
    [[nodiscard]] bool fully_equals(const Declaration& rhs) noexcept {
      return *this == rhs && loc() == rhs.loc();
    }

    /// Clones the node and returns a `unique_ptr` to the copy of the node
    ///
    /// \return A new node with the same observable state
    [[nodiscard]] std::unique_ptr<Declaration> clone() const noexcept {
      return internal_clone();
    }

    /// Checks if a node is of a particular type in slightly
    /// nicer form than `.type() ==`
    ///
    /// \param type The type to compare against
    /// \return Whether or not the node is of that type
    [[nodiscard]] bool is(DeclType type) const noexcept {
      return real_ == type;
    }

    /// Checks if a node is one of a set of types
    ///
    /// \tparam Args The list of types
    /// \param types The types to check against
    /// \return Whether or not the node is of one of those types
    template <typename... Args> [[nodiscard]] bool is_one_of(Args... types) const noexcept {
      return (is(types) || ...);
    }

    [[nodiscard]] bool injected() const noexcept {
      return injected_;
    }

    void set_injected() noexcept {
      injected_ = true;
    }

  protected:
    /// Initializes the state of the declaration base class
    ///
    /// \param exported Whether or not this particular declaration is marked `export`
    explicit Declaration(SourceLoc loc, bool exported, DeclType real) noexcept
        : Node(std::move(loc)),
          exported_{exported},
          injected_{false},
          real_{real} {}

    /// Protected so only derived can copy
    Declaration(const Declaration&) = default;

    /// Protected so only derived can move
    Declaration(Declaration&&) = default;

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void internal_accept(DeclarationVisitorBase* visitor) = 0;

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void internal_accept(ConstDeclarationVisitorBase* visitor) const = 0;

    /// Compares two decl objects of the same underlying type for equality
    ///
    /// \param other The other node to compare
    /// \return Whether the nodes are equal
    [[nodiscard]] virtual bool internal_equals(const Declaration& other) const noexcept = 0;

    /// Creates a clone of the node where `clone()` is called on
    ///
    /// \return A node equal in all observable ways
    [[nodiscard]] virtual std::unique_ptr<Declaration> internal_clone() const noexcept = 0;

  private:
    bool exported_;
    bool injected_;
    DeclType real_;
  };

  /// Models a plain `import` declaration of the form `import foo::bar`.
  ///
  /// Simply contains the module imported and nothing else.
  class ImportDeclaration final : public Declaration {
  public:
    /// Constructs an `ImportDeclaration`
    ///
    /// \param exported Whether or not the import is `export`ed
    /// \param module_imported The module being imported
    explicit ImportDeclaration(SourceLoc loc,
        bool exported,
        ModuleID module_imported,
        std::optional<std::string> alias) noexcept
        : Declaration(std::move(loc), exported, DeclType::import_decl),
          mod_{std::move(module_imported)},
          alias_{std::move(alias)} {}

    /// Gets the module that is being imported
    ///
    /// \return The module being imported
    [[nodiscard]] const ModuleID& mod() const noexcept {
      return mod_;
    }

    /// If the import has an `as` clause, returns the alias. Otherwise returns
    /// an empty optional
    ///
    /// \return An alias name, or an empty optional
    [[nodiscard]] std::optional<std::string_view> alias() const noexcept {
      if (alias_.has_value()) {
        return std::make_optional(*alias_);
      } else {
        return std::nullopt;
      }
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    ModuleID mod_;
    std::optional<std::string> alias_;
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
    explicit ImportFromDeclaration(SourceLoc loc, bool exported, std::vector<FullyQualifiedID> entities) noexcept
        : Declaration(std::move(loc), exported, DeclType::import_from_decl),
          entities_{std::move(entities)} {}

    /// Gets a list of the entities imported
    ///
    /// \return Every name that is imported by the declaration
    [[nodiscard]] absl::Span<const FullyQualifiedID> imported_entities() const noexcept {
      return entities_;
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    std::vector<FullyQualifiedID> entities_;
  };

  /// Functions can be marked with attributes
  enum class AttributeType {
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
    builtin_stdlib,        // __stdlib
    builtin_varargs,       // __varargs
  };

  /// An attribute can have other arguments given to it
  struct Attribute {
    AttributeType type;
    std::vector<std::string> args;

    [[nodiscard]] friend bool operator==(const Attribute& lhs, const Attribute& rhs) noexcept {
      return lhs.type == rhs.type && lhs.args == rhs.args;
    }
  };

  /// An argument is a `name: type` pair
  class Argument {
  public:
    /// Creates an argument
    ///
    /// \param name The name given
    /// \param type The type of the argument
    explicit Argument(SourceLoc loc, std::string name, std::unique_ptr<Type> type) noexcept
        : loc_{std::move(loc)},
          name_{std::move(name)},
          type_{std::move(type)} {}

    /// Properly copies an argument
    Argument(const Argument& other) noexcept : loc_{other.loc_}, name_{other.name_}, type_{other.type_->clone()} {}

    /// Moves an argument
    Argument(Argument&&) noexcept = default;

    /// Gets the location of the arg in the source
    ///
    /// \return The location
    [[nodiscard]] const SourceLoc& loc() const noexcept {
      return loc_;
    }

    /// Gets the name of the argument
    //
    /// \return The name
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets the type of the argument
    ///
    /// \return The type of the argument
    [[nodiscard]] const Type& type() const noexcept {
      return *type_;
    }

    /// Gets the type of the argument
    ///
    /// \return The type of the argument
    [[nodiscard]] Type* type_mut() noexcept {
      return type_.get();
    }

    /// Gets the type of the argument
    ///
    /// \return The type of the argument
    [[nodiscard]] std::unique_ptr<Type>* type_owner() noexcept {
      return &type_;
    }

    /// Compares two arguments
    ///
    /// \param lhs The left argument
    /// \param rhs The right argument
    /// \return Whether or not they're equal
    [[nodiscard]] friend bool operator==(const Argument& lhs, const Argument& rhs) noexcept {
      return lhs.name() == rhs.name() && lhs.type() == rhs.type();
    }

  private:
    SourceLoc loc_;
    std::string name_;
    std::unique_ptr<Type> type_;
  };

  /// Maps the 4 types of `self` that a method is able to take
  enum class SelfType {
    self_ref,     // &self
    mut_self_ref, // &mut self
    self,         // self
    mut_self,     // mut self
  };

  /// Represents a function prototype, works for methods, interface methods,
  /// function declarations and external function declarations.
  class FnPrototype {
  public:
    FnPrototype() = delete;

    /// Creates a function prototype
    ///
    /// \param name The name of the prototype
    /// \param self The type of self the function takes, if any
    /// \param args The arguments of the function
    /// \param attributes Any attributes the function has on it
    /// \param return_type The return type of the function
    explicit FnPrototype(std::string name,
        std::optional<SelfType> self,
        std::vector<Argument> args,
        std::vector<Attribute> attributes,
        std::unique_ptr<Type> return_type) noexcept
        : name_{std::move(name)},
          self_{self},
          args_{std::move(args)},
          attributes_{std::move(attributes)},
          return_type_{std::move(return_type)} {}

    /// Copies an FnPrototype
    ///
    /// \param other The FnPrototype to copy from
    FnPrototype(const FnPrototype& other) noexcept
        : name_{other.name_},
          self_{other.self()},
          args_{other.args_},
          attributes_{other.attributes_},
          return_type_{other.return_type().clone()} {}

    /// Moves an fn prototype
    FnPrototype(FnPrototype&&) noexcept = default;

    /// Disables assignment
    FnPrototype& operator=(const FnPrototype&) noexcept = delete;

    /// Disables assignment
    FnPrototype& operator=(FnPrototype&&) noexcept = delete;

    /// Gets the (unmangled) name of the function
    ///
    /// \return The name of the function
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets the type of self, if the prototype has self in it.
    ///
    /// \return The self from the prototype
    [[nodiscard]] std::optional<SelfType> self() const noexcept {
      return self_;
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
    [[nodiscard]] absl::Span<Argument> args_mut() noexcept {
      return {args_.data(), args_.size()};
    }

    /// Gets a list of the function attributes
    ///
    /// \return All function attributes
    [[nodiscard]] absl::Span<const Attribute> attributes() const noexcept {
      return attributes_;
    }

    /// Gets a mutable list of the function attributes
    ///
    /// \return All function attributes
    [[nodiscard]] absl::Span<Attribute> attributes_mut() noexcept {
      return {attributes_.data(), attributes_.size()};
    }

    /// Gets the return_type of the function
    ///
    /// \return The function return_type
    [[nodiscard]] const Type& return_type() const noexcept {
      return *return_type_;
    }

    /// Gets a mutable ptr to the return_type of the function
    ///
    /// \return A mutable ptr to the function return_type
    [[nodiscard]] Type* return_type_mut() noexcept {
      return return_type_.get();
    }

    /// Correctly release and update the return_type for `*this`
    ///
    /// \param return_type The new return_type to use
    [[nodiscard]] std::unique_ptr<Type>* return_type_owner() noexcept {
      return &return_type_;
    }

    /// Compares two FnPrototype objects
    ///
    /// \param lhs The first prototype
    /// \param rhs The second prototype
    /// \return Whether or not they are equal
    friend bool operator==(const FnPrototype& lhs, const FnPrototype& rhs) noexcept;

  private:
    std::string name_;
    std::optional<SelfType> self_;
    std::vector<Argument> args_;
    std::vector<Attribute> attributes_;
    std::unique_ptr<Type> return_type_;
  };

  /// Models a full function declaration, prototype and body.
  class FnDeclaration final : public Declaration, public Mangled {
  public:
    /// Creates a FnDeclaration
    ///
    /// \param exported Whether or not the function is exported
    /// \param external Whether or not the function is marked `extern`
    /// \param proto The function prototype. Must **not** have `self`
    /// \param body The body of the function
    explicit FnDeclaration(SourceLoc loc,
        bool exported,
        bool external,
        FnPrototype proto,
        std::unique_ptr<BlockExpression> body) noexcept
        : Declaration(std::move(loc), exported, DeclType::fn_decl),
          external_{external},
          proto_{std::move(proto)},
          body_{std::move(body)} {
      assert(this->proto().self() == std::nullopt);
    }

    /// Returns if the function is marked `extern`
    ///
    /// \return Whether the function is meant to be visible on FFI boundaries
    [[nodiscard]] bool external() const noexcept {
      return external_;
    }

    /// Gets the prototype of the function
    ///
    /// \return The function prototype
    [[nodiscard]] const FnPrototype& proto() const noexcept {
      return proto_;
    }

    /// Gets mutable access to the prototype of the function
    ///
    /// \return A pointer to the prototype
    [[nodiscard]] FnPrototype* proto_mut() noexcept {
      return &proto_;
    }

    /// Gets the body of the function
    ///
    /// \return The function body
    [[nodiscard]] const BlockExpression& body() const noexcept {
      return gal::as<BlockExpression>(*body_);
    }

    /// Gets a mutable ptr to the body of the function
    ///
    /// \return A mutable ptr to the function body
    [[nodiscard]] BlockExpression* body_mut() const noexcept {
      return gal::as_mut<BlockExpression>(body_.get());
    }

    /// Correctly release and update the body for `*this`
    ///
    /// \param body The new body to use
    [[nodiscard]] std::unique_ptr<Expression>* body_owner() noexcept {
      return &body_;
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    bool external_;
    FnPrototype proto_;
    std::unique_ptr<Expression> body_;
  };

  /// Models a full function declaration, prototype and body.
  class MethodDeclaration final : public Declaration {
  public:
    /// Creates a FnDeclaration
    ///
    /// \param exported Whether or not the function is exported
    /// \param external Whether or not the function is marked `extern`
    /// \param proto The function prototype. **Must** have `self`
    /// \param body The body of the function
    explicit MethodDeclaration(SourceLoc loc,
        bool exported,
        FnPrototype proto,
        std::unique_ptr<BlockExpression> body) noexcept
        : Declaration(std::move(loc), exported, DeclType::method_decl),
          proto_{std::move(proto)},
          body_{std::move(body)} {
      assert(this->proto().self() != std::nullopt);
    }

    /// Gets the prototype of the method
    ///
    /// \return The function prototype
    [[nodiscard]] const FnPrototype& proto() const noexcept {
      return proto_;
    }

    /// Gets mutable access to the prototype of the method
    ///
    /// \return A pointer to the prototype
    [[nodiscard]] FnPrototype* proto_mut() noexcept {
      return &proto_;
    }

    /// Gets the body of the method
    ///
    /// \return The method body
    [[nodiscard]] const BlockExpression& body() const noexcept {
      return *body_;
    }

    /// Gets a mutable ptr to the body of the method
    ///
    /// \return A mutable ptr to the method body
    [[nodiscard]] BlockExpression* body_mut() const noexcept {
      return body_.get();
    }

    /// Correctly release and update the body for `*this`
    ///
    /// \param body The new body to use
    [[nodiscard]] std::unique_ptr<BlockExpression>* body_owner() noexcept {
      return &body_;
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    FnPrototype proto_;
    std::unique_ptr<BlockExpression> body_;
  };

  /// Error type given when the parser reports an error but still needs to "return" something
  class ErrorDeclaration final : public Declaration {
  public:
    /// Creates a fake declaration
    explicit ErrorDeclaration() : Declaration(SourceLoc::nonexistent(), false, DeclType::error_decl) {}

  protected:
    void internal_accept(DeclarationVisitorBase*) final {
      assert(false);
    }

    void internal_accept(ConstDeclarationVisitorBase*) const final {
      assert(false);
    }

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final {
      (void)gal::as<ErrorDeclaration>(other);

      return true;
    }

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final {
      return std::make_unique<ErrorDeclaration>();
    }
  };
  /// Models a single field in a struct
  class Field {
  public:
    /// Creates a field
    ///
    /// \param name The name of the field
    /// \param type The type of the field
    explicit Field(ast::SourceLoc loc, std::string name, std::unique_ptr<Type> type) noexcept
        : loc_{std::move(loc)},
          name_{std::move(name)},
          type_{std::move(type)} {}

    Field(const Field& other) noexcept : loc_{other.loc_}, name_{other.name_}, type_{other.type_->clone()} {}

    Field(Field&&) = default;

    /// Compares two fields for equality
    ///
    /// \param lhs The left field
    /// \param rhs The right field
    /// \return Whether they are equal
    [[nodiscard]] friend bool operator==(const Field& lhs, const Field& rhs) noexcept {
      return lhs.name() == rhs.name() && lhs.type() == rhs.type();
    }

    /// Gets the name of the field
    ///
    /// \return The name of the field
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets the type of the field
    ///
    /// \return The type of the field
    [[nodiscard]] const Type& type() const noexcept {
      return *type_;
    }

    /// Gets the type of the field
    ///
    /// \return The type of the field
    [[nodiscard]] Type* type_mut() noexcept {
      return type_.get();
    }

    /// Gets the type of the field
    ///
    /// \return The type of the field
    [[nodiscard]] std::unique_ptr<Type>* type_owner() noexcept {
      return &type_;
    }

    /// Gets the location of the field
    ///
    /// \return The location
    [[nodiscard]] const ast::SourceLoc& loc() const noexcept {
      return loc_;
    }

  private:
    ast::SourceLoc loc_;
    std::string name_;
    std::unique_ptr<Type> type_;
  };

  /// Models a `struct` declaration in Gallium
  class StructDeclaration final : public Declaration {
  public:
    /// Creates a struct declaration
    ///
    /// \param loc The location in the source
    /// \param exported Whether or not the struct was exported
    /// \param name The name of the structure
    /// \param fields Every field in the struct
    explicit StructDeclaration(SourceLoc loc, bool exported, std::string name, std::vector<Field> fields) noexcept
        : Declaration(std::move(loc), exported, DeclType::struct_decl),
          name_{std::move(name)},
          fields_{std::move(fields)} {}

    /// Gets the name of the structure
    ///
    /// \return The name of the structure
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets a span over the fields
    ///
    /// \return An immutable span over the fields
    [[nodiscard]] absl::Span<const Field> fields() const noexcept {
      return fields_;
    }

    /// Gets a mutable span over the fields
    ///
    /// \return A span over the fields
    [[nodiscard]] absl::Span<Field> fields_mut() noexcept {
      return absl::MakeSpan(fields_);
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    std::string name_;
    std::vector<Field> fields_;
  };

  /// Models a class declaration
  class ClassDeclaration final : public Declaration {
  public:
    //

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration&) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;
  };

  /// Models a type alias declaration
  class TypeDeclaration final : public Declaration {
  public:
    /// Creates a type alias declaration
    ///
    /// \param loc The location in the source code
    /// \param exported Whether or not it's exported
    /// \param name The name the type is aliased to
    /// \param type The type being aliased
    explicit TypeDeclaration(SourceLoc loc, bool exported, std::string name, std::unique_ptr<Type> type) noexcept
        : Declaration(std::move(loc), exported, DeclType::type_decl),
          name_{std::move(name)},
          type_{std::move(type)} {}

    /// Gets the new name for the aliased type
    ///
    /// \return The new name
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets the type being aliased
    ///
    /// \return The type being aliased
    [[nodiscard]] const Type& aliased() const noexcept {
      return *type_;
    }

    /// Gets the type being aliased
    ///
    /// \return The type being aliased
    [[nodiscard]] Type* aliased_mut() noexcept {
      return type_.get();
    }

    /// Gets a mutable pointer to the *owner* of the type, allowing replacement
    ///
    /// \return A pointer to the aliased type. **Must never be null!**
    [[nodiscard]] std::unique_ptr<Type>* aliased_owner() noexcept {
      return &type_;
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    std::string name_;
    std::unique_ptr<Type> type_;
  };

  /// Maps to an **external** function, i.e one declared within an `external` block
  class ExternalFnDeclaration final : public Declaration, public Mangled {
  public:
    /// Creates an external fn decl
    ///
    /// \param loc The location of the fn
    /// \param exported Whether or not the external block was exported
    /// \param proto The prototype
    explicit ExternalFnDeclaration(SourceLoc loc, bool exported, ast::FnPrototype proto) noexcept
        : Declaration(std::move(loc), exported, DeclType::external_fn_decl),
          proto_{std::move(proto)} {}

    /// Gets the prototype of the fn
    ///
    /// \return The prototype
    [[nodiscard]] const ast::FnPrototype& proto() const noexcept {
      return proto_;
    }

    /// Gets the prototype of the fn
    ///
    /// \return The prototype
    [[nodiscard]] ast::FnPrototype* proto_mut() noexcept {
      return &proto_;
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    ast::FnPrototype proto_;
  };

  /// Models a list of functions that are available over FFI
  class ExternalDeclaration final : public Declaration {
  public:
    /// Creates an ExternalDeclaration
    ///
    /// \param loc The location in the source
    /// \param exported Whether or not it's exported
    /// \param externals The list of external fns
    explicit ExternalDeclaration(SourceLoc loc,
        bool exported,
        std::vector<std::unique_ptr<ast::Declaration>> externals) noexcept
        : Declaration(std::move(loc), exported, DeclType::external_decl),
          externals_{std::move(externals)} {}

    /// Gets the list of external fns
    ///
    /// \return The list of external fns
    [[nodiscard]] absl::Span<const std::unique_ptr<ast::Declaration>> externals() const noexcept {
      return externals_;
    }

    /// Gets the list of external fns
    ///
    /// \return The list of external fns
    [[nodiscard]] absl::Span<std::unique_ptr<ast::Declaration>> externals_mut() noexcept {
      return absl::MakeSpan(externals_);
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    std::vector<std::unique_ptr<ast::Declaration>> externals_;
  };

  /// Models a constant, i.e `const pi: f64 = 3.14159265`
  class ConstantDeclaration final : public Declaration, public Mangled {
  public:
    /// Creates a constant declaration
    ///
    /// \param loc The location of the constant
    /// \param exported Whether or not it was exported
    /// \param name The name of the constant
    /// \param hint The type of the constant
    /// \param init The initializer for the constant
    explicit ConstantDeclaration(ast::SourceLoc loc,
        bool exported,
        std::string name,
        std::unique_ptr<Type> hint,
        std::unique_ptr<Expression> init) noexcept
        : Declaration(std::move(loc), exported, DeclType::constant_decl),
          name_{std::move(name)},
          hint_{std::move(hint)},
          initializer_{std::move(init)} {}

    /// Gets the name of the constant
    ///
    /// \return The name of the constant
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets the type hint of the constant
    ///
    /// \return The type hint
    [[nodiscard]] const Type& hint() const noexcept {
      return *hint_;
    }

    /// Gets the type hint of the constant
    ///
    /// \return The type hint
    [[nodiscard]] Type* hint_mut() noexcept {
      return hint_.get();
    }

    /// Gets the type hint of the constant
    ///
    /// \return The type hint
    [[nodiscard]] std::unique_ptr<Type>* hint_owner() noexcept {
      return &hint_;
    }

    /// Gets the initializer of the constant
    ///
    /// \return The initializer
    [[nodiscard]] const Expression& initializer() const noexcept {
      return *initializer_;
    }

    /// Gets the initializer of the constant
    ///
    /// \return The initializer
    [[nodiscard]] Expression* initializer_mut() noexcept {
      return initializer_.get();
    }

    /// Gets the initializer of the constant
    ///
    /// \return The initializer
    [[nodiscard]] std::unique_ptr<Expression>* initializer_owner() noexcept {
      return &initializer_;
    }

  protected:
    void internal_accept(DeclarationVisitorBase* visitor) final;

    void internal_accept(ConstDeclarationVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Declaration& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Declaration> internal_clone() const noexcept final;

  private:
    std::string name_;
    std::unique_ptr<Type> hint_;
    std::unique_ptr<Expression> initializer_;
  };
} // namespace gal::ast