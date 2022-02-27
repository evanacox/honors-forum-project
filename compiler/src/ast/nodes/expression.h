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

#include "../../utility/log.h"
#include "../../utility/misc.h"
#include "../modular_id.h"
#include "../visitors/expression_visitor.h"
#include "./ast_node.h"
#include "./type.h"
#include "absl/types/span.h"
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace gal {
  class Overload;
}

namespace gal::ast {
  class Statement;
  class Declaration;

  enum class ExprType {
    string_lit,
    integer_lit,
    float_lit,
    bool_lit,
    char_lit,
    nil_lit,
    group,
    identifier,
    identifier_unqualified,
    identifier_local,
    block,
    call,
    static_call,
    method_call,
    static_method_call,
    index,
    field_access,
    unary,
    binary,
    cast,
    if_then,
    if_else,
    loop,
    while_loop,
    for_loop,
    return_expr,
    break_expr,
    continue_expr,
    error_expr,
    struct_expr,
    implicit,
    array,
    load,
    address_of,
    static_global,
    slice_of,
    range_into,
    sizeof_type,
  };

  /// Represents the different unary expression operators
  enum class UnaryOp { logical_not, bitwise_not, ref_to, mut_ref_to, negate, dereference };

  /// Represents the different binary expression operators
  enum class BinaryOp {
    mul,
    div,
    mod,
    add,
    sub,
    lt,
    gt,
    lt_eq,
    gt_eq,
    equals,
    not_equal,
    left_shift,
    right_shift,
    bitwise_and,
    bitwise_or,
    bitwise_xor,
    logical_and,
    logical_or,
    logical_xor,
    assignment,
    add_eq,
    sub_eq,
    mul_eq,
    div_eq,
    mod_eq,
    left_shift_eq,
    right_shift_eq,
    bitwise_and_eq,
    bitwise_or_eq,
    bitwise_xor_eq,
  };

  class Expression : public Node {
  public:
    Expression() = delete;

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Expression() = default;

    /// Compares two expression nodes for equality
    ///
    /// \param lhs The left node to compare
    /// \param rhs The right node to compare
    /// \return Whether or not the nodes are equal
    [[nodiscard]] friend bool operator==(const Expression& lhs, const Expression& rhs) noexcept {
      if (lhs.is(ExprType::error_expr) || rhs.is(ExprType::error_expr)) {
        return true;
      }

      return lhs.type() == rhs.type() && lhs.internal_equals(rhs);
    }

    /// Compares two nodes for inequality
    ///
    /// \param lhs The first node to compare
    /// \param rhs The second node to compare
    /// \return Whether the two are unequal
    [[nodiscard]] friend bool operator!=(const Expression& lhs, const Expression& rhs) noexcept {
      return !(lhs == rhs);
    }

    /// Compares two nodes for inequality
    ///
    /// \param lhs The first node to compare
    /// \param rhs The second node to compare
    /// \return Whether the two are unequal

    /// Compares two expression nodes for complete equality, including source location.
    /// Equivalent to `a == b && a.loc() == b.loc()`
    ///
    /// \param rhs The other node to compare
    /// \return Whether the nodes are identical in every observable way
    [[nodiscard]] bool fully_equals(const Expression& rhs) noexcept {
      return *this == rhs && loc() == rhs.loc();
    }

    /// Gets the real underlying type of the expression
    ///
    /// \return The type of the expression
    [[nodiscard]] ExprType type() const noexcept {
      return real_;
    }

    /// Checks if a node is of a particular type in slightly
    /// nicer form than `.type() ==`
    ///
    /// \param type The type to compare against
    /// \return Whether or not the node is of that type
    [[nodiscard]] bool is(ExprType type) const noexcept {
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

    /// Checks if `.result()` is safe to call
    ///
    /// \return Whether or not `result` is safe to use
    [[nodiscard]] bool has_result() const noexcept {
      return evaluates_to_.has_value();
    }

    /// Clones a node and returns a pointer to the copy
    ///
    /// \return The clone of the node
    [[nodiscard]] std::unique_ptr<Expression> clone() const noexcept {
      auto node = internal_clone();

      if (node->has_result()) {
        node->result_update(node->result().clone());
      }

      return node;
    }

    /// Gets the result type of the expression, i.e the type it will evaluate to
    ///
    /// \return The type that the expression evaluates to
    [[nodiscard]] const Type& result() const noexcept {
      assert(has_result());

      return **evaluates_to_;
    }

    /// Gets a mutable pointer to the result type
    ///
    /// \return Mutable pointer to the result type
    [[nodiscard]] Type* result_mut() noexcept {
      assert(has_result());

      return evaluates_to_->get();
    }

    /// Updates the result type to a new type
    ///
    /// \param new_result The new result type
    void result_update(std::unique_ptr<Type> new_result) noexcept {
      evaluates_to_ = std::move(new_result);
    }

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    void accept(ExpressionVisitorBase* visitor) {
      internal_accept(visitor);
    }

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    void accept(ConstExpressionVisitorBase* visitor) const {
      internal_accept(visitor);
    }

    /// Helper that allows a visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(ExpressionVisitor<T>* visitor) {
      accept(static_cast<ExpressionVisitorBase*>(visitor));

      return visitor->take_result();
    }

    /// Helper that allows a const visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(ConstExpressionVisitor<T>* visitor) const {
      accept(static_cast<ConstExpressionVisitorBase*>(visitor));

      return visitor->take_result();
    }

  protected:
    /// Initializes the base type for `Expression`
    ///
    /// \param real The real expression type
    /// \param evaluates_to The type that the expression evaluates to
    explicit Expression(SourceLoc loc, ExprType real, std::optional<std::unique_ptr<Type>> evaluates_to = std::nullopt)
        : Node(std::move(loc)),
          real_{real},
          evaluates_to_{std::move(evaluates_to)} {}

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void internal_accept(ExpressionVisitorBase* visitor) = 0;

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void internal_accept(ConstExpressionVisitorBase* visitor) const = 0;

    /// Compares two expression nodes of the same type for equality
    ///
    /// \param other The other node to compare against. Guaranteed to be the same type as `*this`
    /// \return Whether or not the nodes are equal
    [[nodiscard]] virtual bool internal_equals(const Expression& other) const noexcept = 0;

    /// Clones a node, returns a new node with the same features
    /// and same source location
    ///
    /// \return A new node with the same observable state as the other
    [[nodiscard]] virtual std::unique_ptr<Expression> internal_clone() const noexcept = 0;

  private:
    ExprType real_;
    std::optional<std::unique_ptr<Type>> evaluates_to_;
  };

  /// Represents a "string literal," i.e `"Hello, World!"`
  class StringLiteralExpression final : public Expression {
  public:
    /// Creates a string literal
    ///
    /// \param text The string literal (with ""s)
    explicit StringLiteralExpression(SourceLoc loc, std::string text) noexcept
        : Expression(std::move(loc), ExprType::string_lit),
          text_{std::move(text)} {}

    /// Gets the full string literal, **including** quotes
    ///
    /// \return The full text
    [[nodiscard]] std::string_view text() const noexcept {
      return text_;
    }

    /// Gets the string literal's text, but without the quotes
    ///
    /// \return [1..len - 1] for `text()`
    [[nodiscard]] std::string_view text_unquoted() const noexcept {
      const auto s = text();

      return s.substr(1, s.size() - 2);
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::string text_;
  };

  /// Represents an integer literal, i.e [0, 2^64 - 1] in digit form
  class IntegerLiteralExpression final : public Expression {
  public:
    /// Creates an integer literal
    ///
    /// \param value The value of the literal
    explicit IntegerLiteralExpression(SourceLoc loc, std::uint64_t value) noexcept
        : Expression(std::move(loc), ExprType::integer_lit),
          literal_{value} {};

    /// Gets the value of the integer literal
    ///
    /// \return The value of the integer literal
    [[nodiscard]] std::uint64_t value() const noexcept {
      return literal_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::uint64_t literal_;
  };

  /// Models a floating-point literal
  class FloatLiteralExpression final : public Expression {
  public:
    // on every computer that matters double is IEEE-754, can't hurt to check though
    static_assert(std::numeric_limits<double>::is_iec559);

    /// Initializes a floating-point literal
    ///
    /// \param loc The location in the source
    /// \param lit The actual value parsed
    /// \param str_len The number of characters in the string representation it was parsed from
    explicit FloatLiteralExpression(SourceLoc loc, double lit, std::size_t str_len) noexcept
        : Expression(std::move(loc), ExprType::float_lit),
          literal_{lit},
          str_len_{str_len} {}

    /// Gets the value of the literal
    ///
    /// \return The literal
    [[nodiscard]] double value() const noexcept {
      return literal_;
    }

    /// Gets the string length of the lit
    ///
    /// \return The string length
    [[nodiscard]] std::size_t str_len() const noexcept {
      return str_len_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    double literal_;
    std::size_t str_len_;
  };

  /// Models a boolean literal, i.e `true` / `false`
  class BoolLiteralExpression final : public Expression {
  public:
    /// Creates a boolean literal expression
    ///
    /// \param loc The location in the source code
    /// \param value The boolean value
    explicit BoolLiteralExpression(SourceLoc loc, bool value) noexcept
        : Expression(std::move(loc), ExprType::bool_lit),
          literal_{value} {}

    /// Gets the value of the literal
    ///
    /// \return The value of the literal
    [[nodiscard]] bool value() const noexcept {
      return literal_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    bool literal_;
  };

  /// Models a character literal of some sort
  class CharLiteralExpression final : public Expression {
  public:
    /// Creates a character literal
    ///
    /// \param loc The location in the source code
    /// \param value The value as a byte
    explicit CharLiteralExpression(SourceLoc loc, std::uint8_t value) noexcept
        : Expression(std::move(loc), ExprType::char_lit),
          literal_{value} {}

    /// The actual value of the literal, ready to get embedded in the IR
    ///
    /// \return The value
    [[nodiscard]] std::uint8_t value() const noexcept {
      return literal_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::uint8_t literal_;
  };

  /// Models a `nil` literal
  class NilLiteralExpression final : public Expression {
  public:
    /// Creates a nil literal
    ///
    /// \param loc The location in the source it came from
    explicit NilLiteralExpression(SourceLoc loc) noexcept : Expression(std::move(loc), ExprType::nil_lit) {}

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;
  };

  /// Represents a single portion of the id list
  struct NestedGenericID {
    /// The name of the nested ID, i.e `Foo` in `::Foo<i32>`
    std::string name;
    /// The list of generic parameters, i.e `{i32, i32}` in `::Pair<i32, i32>`
    std::vector<std::unique_ptr<Type>> ids;

    /// Copies a NestedGenericID by cloning the members
    ///
    /// \param other The other ID to copy
    NestedGenericID(const NestedGenericID& other) : name{other.name}, ids{internal::clone_generics(other.ids)} {}

    /// Moves a NestedGenericID
    NestedGenericID(NestedGenericID&&) = default;

    NestedGenericID& operator=(const NestedGenericID&) = delete;

    NestedGenericID& operator=(NestedGenericID&&) = delete;

    /// Compares two generic IDs
    ///
    /// \param lhs The first ID
    /// \param rhs The second ID
    /// \return Whether or not they're equal
    friend bool operator==(const NestedGenericID& lhs, const NestedGenericID& rhs) noexcept;
  };

  /// Models the **nested** and possibly-generic identifiers
  ///
  /// E.g `Foo<i32>::Bar::baz` in `Vec<i32>::Foo<i32>::Bar::baz`
  class NestedGenericIDList final {
  public:
    /// Initializes a generic identifier list
    ///
    /// \param ids The generic IDs
    explicit NestedGenericIDList(std::vector<NestedGenericID> ids) noexcept : ids_{std::move(ids)} {}

    /// Copies a GenericIDList
    NestedGenericIDList(const NestedGenericIDList&) noexcept = default;

    /// Moves a GenericIDList
    NestedGenericIDList(NestedGenericIDList&&) noexcept = default;

    /// Gets the ids making up the list
    ///
    /// \return The ids in the list
    [[nodiscard]] absl::Span<const NestedGenericID> ids() const noexcept {
      return ids_;
    }

    /// Gets the ids making up the list
    ///
    /// \return The ids in the list
    [[nodiscard]] absl::Span<NestedGenericID> ids_mut() noexcept {
      return absl::MakeSpan(ids_);
    }

    /// Checks if two generic ID lists have equivalent ID lists
    ///
    /// \param lhs The first list to compare
    /// \param rhs The second list to compare
    /// \return Whether the ID lists are equal
    friend bool operator==(const NestedGenericIDList& lhs, const NestedGenericIDList& rhs) noexcept;

  private:
    std::vector<NestedGenericID> ids_;
  };

  /// Models a fully qualified identifier, only used for things that don't refer
  /// to locals.
  class IdentifierExpression final : public Expression {
  public:
    /// Creates a fully qualified identifier
    ///
    /// \param loc The location
    /// \param id The fully-qualified id
    explicit IdentifierExpression(SourceLoc loc,
        FullyQualifiedID id,
        std::vector<std::unique_ptr<Type>> generic_params = {},
        std::optional<NestedGenericIDList> list = {}) noexcept
        : Expression(std::move(loc), ExprType::identifier),
          id_{std::move(id)},
          generic_params_{std::move(generic_params)},
          nested_{std::move(list)} {}

    /// Gets the fully-qualified ID
    ///
    /// \return The ID
    [[nodiscard]] const FullyQualifiedID& id() const noexcept {
      return id_;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

    /// Gets the nested identifiers, if they exist
    ///
    /// \return Nested identifiers
    [[nodiscard]] std::optional<const NestedGenericIDList*> nested() const noexcept {
      return (nested_.has_value()) ? std::make_optional(&*nested_) : std::nullopt;
    }

    /// Gets the nested identifiers, if they exist
    ///
    /// \return Nested identifiers
    [[nodiscard]] std::optional<NestedGenericIDList*> nested_mut() noexcept {
      return (nested_.has_value()) ? std::make_optional(&*nested_) : std::nullopt;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    FullyQualifiedID id_;
    std::vector<std::unique_ptr<Type>> generic_params_;
    std::optional<NestedGenericIDList> nested_;
  };

  /// Models the idea of a "local" identifier, i.e one local to the function scope
  class LocalIdentifierExpression final : public Expression {
  public:
    /// Creates a local identifier
    ///
    /// \param loc The location
    /// \param id The fully-qualified id
    explicit LocalIdentifierExpression(SourceLoc loc, std::string name) noexcept
        : Expression(std::move(loc), ExprType::identifier_local),
          name_{std::move(name)} {}

    /// Gets the fully-qualified ID
    ///
    /// \return The ID
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::string name_;
  };

  /// Represents a normal identifier, e.g `a` or `foo::bar`.
  class UnqualifiedIdentifierExpression final : public Expression {
  public:
    /// Creates a fully qualified identifier
    ///
    /// \param loc The location
    /// \param id The fully-qualified id
    explicit UnqualifiedIdentifierExpression(SourceLoc loc,
        UnqualifiedID id,
        std::vector<std::unique_ptr<Type>> generic_params,
        std::optional<NestedGenericIDList> nested_generics) noexcept
        : Expression(std::move(loc), ExprType::identifier_unqualified),
          id_{std::move(id)},
          generic_params_{std::move(generic_params)},
          nested_{std::move(nested_generics)} {}

    /// Gets the fully-qualified ID
    ///
    /// \return The ID
    [[nodiscard]] const UnqualifiedID& id() const noexcept {
      return id_;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

    /// Gets the nested identifiers, if they exist
    ///
    /// \return Nested identifiers
    [[nodiscard]] std::optional<const NestedGenericIDList*> nested() const noexcept {
      return (nested_.has_value()) ? std::make_optional(&*nested_) : std::nullopt;
    }

    /// Gets the nested identifiers, if they exist
    ///
    /// \return Nested identifiers
    [[nodiscard]] std::optional<NestedGenericIDList*> nested_mut() noexcept {
      return (nested_.has_value()) ? std::make_optional(&*nested_) : std::nullopt;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    UnqualifiedID id_;
    std::vector<std::unique_ptr<Type>> generic_params_;
    std::optional<NestedGenericIDList> nested_;
  };

  namespace internal {
    struct CopyOverloadWithoutDefinition {
    public:
      explicit CopyOverloadWithoutDefinition(const Overload& overload);

      CopyOverloadWithoutDefinition(const CopyOverloadWithoutDefinition&);

      CopyOverloadWithoutDefinition(CopyOverloadWithoutDefinition&&) noexcept = default;

      ~CopyOverloadWithoutDefinition();

      [[nodiscard]] const Overload& overload() const noexcept {
        return *overload_;
      }

    private:
      std::unique_ptr<Overload> overload_;
    };
  } // namespace internal

  /// Models
  class StaticCallExpression final : public Expression {
  public:
    /// Creates a call expression
    ///
    /// \param loc The location in the source it comes from
    /// \param callee The object / expression being called
    /// \param args Any arguments given to the call
    explicit StaticCallExpression(SourceLoc loc,
        const Overload& callee,
        ast::FullyQualifiedID id,
        std::vector<std::unique_ptr<Expression>> args,
        std::vector<std::unique_ptr<Type>> generic_args)
        : Expression(std::move(loc), ExprType::static_call),
          id_{std::move(id)},
          callee_{callee},
          args_{std::move(args)},
          generic_params_{std::move(generic_args)} {}

    /// Gets the ID being called
    ///
    /// \return The ID being called
    [[nodiscard]] const ast::FullyQualifiedID& id() const noexcept {
      return id_;
    }

    /// Gets the object being called
    ///
    /// \return The object being called
    [[nodiscard]] const Overload& callee() const noexcept {
      return callee_.overload();
    }

    /// Gets the arguments for the call
    ///
    /// \return The arguments to the call
    [[nodiscard]] absl::Span<const std::unique_ptr<Expression>> args() const noexcept {
      return args_;
    }

    /// Gets the arguments for the call
    ///
    /// \return The arguments to the call
    [[nodiscard]] absl::Span<std::unique_ptr<Expression>> args_mut() noexcept {
      return absl::MakeSpan(args_);
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

    /// Steals from a regular call to more efficiently "qualify" a call
    ///
    /// \param id The ID of the new call
    /// \param overload The overload
    /// \param call The call to steal from
    /// \return A static call
    static std::unique_ptr<StaticCallExpression> from_call(const ast::FullyQualifiedID& id,
        const Overload& overload,
        class CallExpression* call) noexcept;

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    ast::FullyQualifiedID id_;
    internal::CopyOverloadWithoutDefinition callee_;
    std::vector<std::unique_ptr<Expression>> args_;
    std::vector<std::unique_ptr<Type>> generic_params_;
  };

  /// Models a call expression, contains both the callee and the call arguments
  class CallExpression final : public Expression {
  public:
    /// Creates a call expression
    ///
    /// \param loc The location in the source it comes from
    /// \param callee The object / expression being called
    /// \param args Any arguments given to the call
    explicit CallExpression(SourceLoc loc,
        std::unique_ptr<Expression> callee,
        std::vector<std::unique_ptr<Expression>> args,
        std::vector<std::unique_ptr<Type>> generic_args)
        : Expression(std::move(loc), ExprType::call),
          callee_{std::move(callee)},
          args_{std::move(args)},
          generic_params_{std::move(generic_args)} {}

    /// Gets the object being called
    ///
    /// \return The object being called
    [[nodiscard]] const Expression& callee() const noexcept {
      return *callee_;
    }

    /// Gets the object being called
    ///
    /// \return The object being called
    [[nodiscard]] Expression* callee_mut() noexcept {
      return callee_.get();
    }

    /// Gets the owner of the callee
    ///
    /// \return A pointer to the owner of the callee
    [[nodiscard]] std::unique_ptr<Expression>* callee_owner() noexcept {
      return &callee_;
    }

    /// Gets the arguments for the call
    ///
    /// \return The arguments to the call
    [[nodiscard]] absl::Span<const std::unique_ptr<Expression>> args() const noexcept {
      return args_;
    }

    /// Gets the arguments for the call
    ///
    /// \return The arguments to the call
    [[nodiscard]] absl::Span<std::unique_ptr<Expression>> args_mut() noexcept {
      return absl::MakeSpan(args_);
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    friend class StaticCallExpression;

    std::unique_ptr<Expression> callee_;
    std::vector<std::unique_ptr<Expression>> args_;
    std::vector<std::unique_ptr<Type>> generic_params_;
  };

  /// Models a call expression, contains both the callee and the call arguments
  class MethodCallExpression final : public Expression {
  public:
    /// Creates a method call expression
    ///
    /// \param loc The location in the source it comes from
    /// \param object The object / expression used for `self`
    /// \param method_name The name of the method being called
    /// \param args Any arguments given to the call
    explicit MethodCallExpression(SourceLoc loc,
        std::unique_ptr<Expression> object,
        std::string method_name,
        std::vector<std::unique_ptr<Expression>> args,
        std::vector<std::unique_ptr<Type>> generic_params)
        : Expression(std::move(loc), ExprType::method_call),
          object_{std::move(object)},
          method_name_{std::move(method_name)},
          args_{std::move(args)},
          generic_params_{std::move(generic_params)} {}

    /// Gets the object being called
    ///
    /// \return The object being called
    [[nodiscard]] const Expression& object() const noexcept {
      return *object_;
    }

    /// Gets the object being called
    ///
    /// \return The object being called
    [[nodiscard]] Expression* object_mut() noexcept {
      return object_.get();
    }

    /// Gets the owner of the callee
    ///
    /// \return A pointer to the owner of the callee
    [[nodiscard]] std::unique_ptr<Expression>* object_owner() noexcept {
      return &object_;
    }

    /// Gets the name of the method being called
    ///
    /// \return The name of the method being called
    [[nodiscard]] std::string_view method_name() const noexcept {
      return method_name_;
    }

    /// Gets the arguments for the call
    ///
    /// \return The arguments to the call
    [[nodiscard]] absl::Span<const std::unique_ptr<Expression>> args() const noexcept {
      return args_;
    }

    /// Gets the arguments for the call
    ///
    /// \return The arguments to the call
    [[nodiscard]] std::vector<std::unique_ptr<Expression>>* args_mut() noexcept {
      return &args_;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> object_;
    std::string method_name_;
    std::vector<std::unique_ptr<Expression>> args_;
    std::vector<std::unique_ptr<Type>> generic_params_;
  };

  enum class Range { inclusive, exclusive };

  /// Represents an index expression, ie `a[b]`
  class IndexExpression final : public Expression {
  public:
    /// Creates an index expression
    ///
    /// \param loc The location in the source code
    /// \param callee The object being indexed into
    /// \param args The argument(s) to the index
    explicit IndexExpression(SourceLoc loc, std::unique_ptr<Expression> callee, std::unique_ptr<Expression> index)
        : Expression(std::move(loc), ExprType::index),
          callee_{std::move(callee)},
          index_{std::move(index)} {}

    /// Gets the expression being indexed into
    ///
    /// \return The expression being indexed into
    [[nodiscard]] const Expression& callee() const noexcept {
      return *callee_;
    }

    /// Gets the expression being indexed into
    ///
    /// \return The expression being indexed into
    [[nodiscard]] Expression* callee_mut() noexcept {
      return callee_.get();
    }

    /// Gets the owner of the expression being indexed into
    ///
    /// \return The expression being indexed into
    [[nodiscard]] std::unique_ptr<Expression>* callee_owner() noexcept {
      return &callee_;
    }

    [[nodiscard]] const Expression& index() const noexcept {
      return *index_;
    }

    [[nodiscard]] Expression* index_mut() noexcept {
      return index_.get();
    }

    [[nodiscard]] std::unique_ptr<Expression>* index_owner() noexcept {
      return &index_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> callee_;
    std::unique_ptr<Expression> index_;
  };

  /// Represents a field access expression, ile `a.b`
  class FieldAccessExpression final : public Expression {
  public:
    /// Creates a field access expression
    ///
    /// \param loc The location in the source
    /// \param object The expression being accessed
    /// \param field The name of the field
    explicit FieldAccessExpression(SourceLoc loc, std::unique_ptr<Expression> object, std::string field) noexcept
        : Expression(std::move(loc), ExprType::field_access),
          object_{std::move(object)},
          field_{std::move(field)},
          slice_{false} {}

    /// Gets the object being accessed
    ///
    /// \return The object having a field accessed
    [[nodiscard]] const Expression& object() const noexcept {
      return *object_;
    }

    /// Gets the object being accessed
    ///
    /// \return The object having a field accessed
    [[nodiscard]] Expression* object_mut() noexcept {
      return object_.get();
    }

    /// Gets the owner of the object being accessed
    ///
    /// \return The owner of the object having a field accessed
    [[nodiscard]] std::unique_ptr<Expression>* object_owner() noexcept {
      return &object_;
    }

    /// Gets the object being accessed
    ///
    /// \return The object having a field accessed
    [[nodiscard]] std::string_view field_name() const noexcept {
      return field_;
    }

    void annotate_type(std::unique_ptr<Type> type) noexcept {
      user_type_ = std::move(type);
    }

    [[nodiscard]] const ast::Type& user_type() const noexcept {
      return *user_type_;
    }

    [[nodiscard]] bool slice_access() const noexcept {
      return slice_;
    }

    void set_slice() noexcept {
      slice_ = true;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& expression) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> object_;
    std::unique_ptr<Type> user_type_;
    std::string field_;
    bool slice_;
  };

  /// Represents a grouped expression, i.e `(a + b)`
  class GroupExpression final : public Expression {
  public:
    /// Creates a group expression
    ///
    /// \param loc The location in the source
    /// \param grouped The expression inside the ()s
    explicit GroupExpression(SourceLoc loc, std::unique_ptr<Expression> grouped) noexcept
        : Expression(std::move(loc), ExprType::group),
          grouped_{std::move(grouped)} {}

    /// Gets the expression inside the ()s
    ///
    /// \return The expression inside the ()s
    [[nodiscard]] const Expression& expr() const noexcept {
      return *grouped_;
    }

    /// Gets the expression inside the ()s
    ///
    /// \return The expression inside the ()s
    [[nodiscard]] Expression* expr_mut() noexcept {
      return grouped_.get();
    }

    /// Gets the owner of the expression inside the ()s
    ///
    /// \return The owner of the expression inside the ()s
    [[nodiscard]] std::unique_ptr<Expression>* expr_owner() noexcept {
      return &grouped_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> grouped_;
  };

  /// Represents a unary expression
  class UnaryExpression final : public Expression {
  public:
    /// Creates a unary expression
    ///
    /// \param loc The location in the source code
    /// \param op The operator being applied to expr
    /// \param expr The expression being operated on
    explicit UnaryExpression(SourceLoc loc, UnaryOp op, std::unique_ptr<Expression> expr)
        : Expression(std::move(loc), ExprType::unary),
          expr_{std::move(expr)},
          op_{op} {}

    /// Gets the expression being operated on
    ///
    /// \return The expression being operated on
    [[nodiscard]] const Expression& expr() const noexcept {
      return *expr_;
    }

    /// Gets the expression being operated on
    ///
    /// \return The expression being operated on
    [[nodiscard]] Expression* expr_mut() noexcept {
      return expr_.get();
    }

    /// Gets the owner of the expression being operated on
    ///
    /// \return The owner of the expression being operated on
    [[nodiscard]] std::unique_ptr<Expression>* expr_owner() noexcept {
      return &expr_;
    }

    /// Gets the unary operation being applied to the expression
    ///
    /// \return The unary operator
    [[nodiscard]] UnaryOp op() const noexcept {
      return op_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> expr_;
    UnaryOp op_;
  };

  /// Models a binary expression
  class BinaryExpression final : public Expression {
  public:
    /// Creates a binary expression
    ///
    /// \param loc The location in the source code
    /// \param op The operator
    /// \param lhs The left hand side of the expression
    /// \param rhs The right hand side of the expression
    explicit BinaryExpression(SourceLoc loc,
        BinaryOp op,
        std::unique_ptr<Expression> lhs,
        std::unique_ptr<Expression> rhs)
        : Expression(std::move(loc), ExprType::binary),
          lhs_{std::move(lhs)},
          rhs_{std::move(rhs)},
          op_{op} {}

    /// Gets the left hand side of the bin expression
    ///
    /// \return The left hand side of the expression
    [[nodiscard]] const Expression& lhs() const noexcept {
      return *lhs_;
    }

    /// Gets the left hand side of the bin expression
    ///
    /// \return The left hand side of the expression
    [[nodiscard]] Expression* lhs_mut() noexcept {
      return lhs_.get();
    }

    /// Gets the owner of the left hand side of the bin expression
    ///
    /// \return The owner of the left hand side of the expression
    [[nodiscard]] std::unique_ptr<Expression>* lhs_owner() noexcept {
      return &lhs_;
    }

    /// Gets the right hand side of the bin expression
    ///
    /// \return The right hand side of the expression
    [[nodiscard]] const Expression& rhs() const noexcept {
      return *rhs_;
    }

    /// Gets the right hand side of the bin expression
    ///
    /// \return The right hand side of the expression
    [[nodiscard]] Expression* rhs_mut() noexcept {
      return rhs_.get();
    }

    /// Gets the owner of the right hand side of the bin expression
    ///
    /// \return The owner of the right hand side of the expression
    [[nodiscard]] std::unique_ptr<Expression>* rhs_owner() noexcept {
      return &rhs_;
    }

    /// Gets the binary operator for the expression
    ///
    /// \return The binary operator
    [[nodiscard]] BinaryOp op() const noexcept {
      return op_;
    }

    /// Checks if the binary operator is one of the **compound** assignment operators
    ///
    /// \return Whether `op()` is a compound assignment op
    [[nodiscard]] bool is_compound_assignment() const noexcept {
      switch (op()) {
        case ast::BinaryOp::add_eq:
        case ast::BinaryOp::sub_eq:
        case ast::BinaryOp::mul_eq:
        case ast::BinaryOp::div_eq:
        case ast::BinaryOp::mod_eq:
        case ast::BinaryOp::left_shift_eq:
        case ast::BinaryOp::right_shift_eq:
        case ast::BinaryOp::bitwise_and_eq:
        case ast::BinaryOp::bitwise_or_eq:
        case ast::BinaryOp::bitwise_xor_eq: return true;
        default: return false;
      }
    }

    /// Checks if the binary operator is one of the ordering operators
    ///
    /// \return Whether `op()` is an ordering operator
    [[nodiscard]] bool is_ordering() const noexcept {
      return op() == ast::BinaryOp::lt || op() == ast::BinaryOp::gt || op() == ast::BinaryOp::lt_eq
             || op() == ast::BinaryOp::gt_eq;
    }

    /// Checks if the binary operator is one of the ordering operators
    ///
    /// \return Whether `op()` is an ordering operator
    [[nodiscard]] bool is_equality() const noexcept {
      return op() == ast::BinaryOp::equals || op() == ast::BinaryOp::not_equal;
    }

    /// Checks if the binary operator is one of the ordering operators
    ///
    /// \return Whether `op()` is an ordering operator
    [[nodiscard]] bool is_logical() const noexcept {
      return op() == ast::BinaryOp::logical_and || op() == ast::BinaryOp::logical_or
             || op() == ast::BinaryOp::logical_xor;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> lhs_;
    std::unique_ptr<Expression> rhs_;
    BinaryOp op_;
  };

  /// Models an `as` or `as!` expression
  class CastExpression final : public Expression {
  public:
    /// Creates a cast expression
    ///
    /// \param loc The source location of the expression
    /// \param unsafe Whether or not the cast is a bitcast
    /// \param castee The object being casted
    /// \param cast_to The type being casted to
    explicit CastExpression(SourceLoc loc,
        bool unsafe,
        std::unique_ptr<Expression> castee,
        std::unique_ptr<Type> cast_to) noexcept
        : Expression(std::move(loc), ExprType::cast),
          unsafe_{unsafe},
          castee_{std::move(castee)},
          cast_to_{std::move(cast_to)} {}

    /// Checks if the cast is an unsafe bitcast or not
    ///
    /// \return True if the cast is a bitcast
    [[nodiscard]] bool unsafe() const noexcept {
      return unsafe_;
    }

    /// Gets the object being casted
    ///
    /// \return The object being casted
    [[nodiscard]] const Expression& castee() const noexcept {
      return *castee_;
    }

    /// Gets the object being casted
    ///
    /// \return The object being casted
    [[nodiscard]] Expression* castee_mut() noexcept {
      return castee_.get();
    }

    /// Gets the owner of the object being casted
    ///
    /// \return The owner of the object being casted
    [[nodiscard]] std::unique_ptr<Expression>* castee_owner() noexcept {
      return &castee_;
    }

    /// Gets the type that the object is being casted to
    ///
    /// \return The type the object is being casted to
    [[nodiscard]] const Type& cast_to() const noexcept {
      return *cast_to_;
    }

    /// Gets the type that the object is being casted to
    ///
    /// \return The type the object is being casted to
    [[nodiscard]] Type* cast_to_mut() noexcept {
      return cast_to_.get();
    }

    /// Gets the owner of the type that the object is being casted to
    ///
    /// \return The type the owner of the object is being casted to
    [[nodiscard]] std::unique_ptr<Type>* cast_to_owner() noexcept {
      return &cast_to_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    bool unsafe_;
    std::unique_ptr<Expression> castee_;
    std::unique_ptr<Type> cast_to_;
  };

  /// Models a block expression, i.e `{ stmt* }`
  class BlockExpression final : public Expression {
  public:
    /// Creates a block expression
    ///
    /// \param loc The location in the source
    /// \param statements The list of statements inside the block
    explicit BlockExpression(SourceLoc loc, std::vector<std::unique_ptr<Statement>> statements) noexcept;

    /// Gets the statement list for the block
    ///
    /// \return The list of statements inside the block
    [[nodiscard]] absl::Span<const std::unique_ptr<Statement>> statements() const noexcept {
      return statements_;
    }

    /// Gets the statement list for the block
    ///
    /// \return The list of statements inside the block
    [[nodiscard]] absl::Span<std::unique_ptr<Statement>> statements_mut() noexcept {
      return absl::MakeSpan(statements_);
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::vector<std::unique_ptr<Statement>> statements_;
  };

  /// Models an `if a then b else c` expression
  class IfThenExpression final : public Expression {
  public:
    /// Creates an if-then expression
    ///
    /// \param loc The location in the source
    /// \param condition The condition of the expression
    /// \param true_branch The expression to evaluate to when the condition is true
    /// \param false_branch The expression to evaluate to when the condition is false
    explicit IfThenExpression(SourceLoc loc,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Expression> true_branch,
        std::unique_ptr<Expression> false_branch) noexcept
        : Expression(std::move(loc), ExprType::if_then),
          condition_{std::move(condition)},
          true_branch_{std::move(true_branch)},
          false_branch_{std::move(false_branch)} {}

    /// Gets the condition of the if
    ///
    /// \return The condition of the if
    [[nodiscard]] const Expression& condition() const noexcept {
      return *condition_;
    }

    /// Gets the condition of the if
    ///
    /// \return The condition of the if
    [[nodiscard]] Expression* condition_mut() noexcept {
      return condition_.get();
    }

    /// Gets the owner of the condition of the if
    ///
    /// \return The owner of the condition of the if
    [[nodiscard]] std::unique_ptr<Expression>* condition_owner() noexcept {
      return &condition_;
    }

    /// Gets the expression on the true side
    ///
    /// \return The true expression
    [[nodiscard]] const Expression& true_branch() const noexcept {
      return *true_branch_;
    }

    /// Gets the expression on the true side
    ///
    /// \return The true expression
    [[nodiscard]] Expression* true_branch_mut() noexcept {
      return true_branch_.get();
    }

    /// Gets the owner of the expression on the true side
    ///
    /// \return The owner of the true expression
    [[nodiscard]] std::unique_ptr<Expression>* true_branch_owner() noexcept {
      return &true_branch_;
    }

    /// Gets the expression on the false side
    ///
    /// \return The false expression
    [[nodiscard]] const Expression& false_branch() const noexcept {
      return *false_branch_;
    }

    /// Gets the expression on the false side
    ///
    /// \return The false expression
    [[nodiscard]] Expression* false_branch_mut() noexcept {
      return false_branch_.get();
    }

    /// Gets the owner of the expression on the false side
    ///
    /// \return The owner of the false expression
    [[nodiscard]] std::unique_ptr<Expression>* false_branch_owner() noexcept {
      return &false_branch_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Expression> true_branch_;
    std::unique_ptr<Expression> false_branch_;
  };
  /// Models a single `elif` in a chain
  struct ElifBlock {
    explicit ElifBlock(std::unique_ptr<Expression> cond, std::unique_ptr<BlockExpression> block) noexcept
        : condition_{std::move(cond)},
          block_{std::move(block)} {}

    /// Copies an ElifBlock by cloning both members
    ///
    /// \param other The elifblock to clone
    ElifBlock(const ElifBlock& other) noexcept;

    /// Moves an ElifBlock
    ElifBlock(ElifBlock&&) = default;

    /// Checks two blocks for equivalency
    ///
    /// \param lhs The left block
    /// \param rhs The right block
    /// \return Whether they are equivalent
    [[nodiscard]] friend bool operator==(const ElifBlock& lhs, const ElifBlock& rhs) noexcept {
      return lhs.condition() == rhs.condition() && lhs.block() == rhs.block();
    }

    /// Gets the condition of the elif
    ///
    /// \return The condition
    [[nodiscard]] const Expression& condition() const noexcept {
      return *condition_;
    }

    /// Gets the condition of the elif
    ///
    /// \return The condition
    [[nodiscard]] Expression* condition_mut() noexcept {
      return condition_.get();
    }

    /// Gets the condition of the elif
    ///
    /// \return The condition
    [[nodiscard]] std::unique_ptr<Expression>* condition_owner() noexcept {
      return &condition_;
    }

    /// Gets the elif's block
    ///
    /// \return The block
    [[nodiscard]] const BlockExpression& block() const noexcept {
      return gal::as<BlockExpression>(*block_);
    }

    /// Gets the elif's block
    ///
    /// \return The block
    [[nodiscard]] BlockExpression* block_mut() noexcept {
      return gal::as_mut<BlockExpression>(block_.get());
    }

    /// Gets the elif's block
    ///
    /// \return The block
    [[nodiscard]] std::unique_ptr<Expression>* block_owner() noexcept {
      return &block_;
    }

  private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Expression> block_;
  };

  /// Models an if-elif-else chain
  class IfElseExpression final : public Expression {
  public:
    /// Creates an if-else block expression
    ///
    /// \param loc The location in the source code
    /// \param condition The condition to check
    /// \param block The block to enter if the condition is true
    /// \param elif_blocks Any elifs, if they exist
    /// \param else_block The else block, if it exists
    explicit IfElseExpression(SourceLoc loc,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Expression> block,
        std::vector<ElifBlock> elif_blocks,
        std::optional<std::unique_ptr<Expression>> else_block) noexcept
        : Expression(std::move(loc), ExprType::if_else),
          condition_{std::move(condition)},
          block_{std::move(block)},
          elif_blocks_{std::move(elif_blocks)},
          else_block_{std::move(else_block)} {}

    /// Gets the condition of the if
    ///
    /// \return The condition of the if
    [[nodiscard]] const Expression& condition() const noexcept {
      return *condition_;
    }

    /// Gets the condition of the if
    ///
    /// \return The condition of the if
    [[nodiscard]] Expression* condition_mut() noexcept {
      return condition_.get();
    }

    /// Gets the owner of the condition of the if
    ///
    /// \return The owner of the condition of the if
    [[nodiscard]] std::unique_ptr<Expression>* condition_owner() noexcept {
      return &condition_;
    }

    /// Gets the block to enter if the condition is true
    ///
    /// \return The true block
    [[nodiscard]] const BlockExpression& block() const noexcept {
      return gal::as<BlockExpression>(*block_);
    }

    /// Gets the block to enter if the condition is true
    ///
    /// \return The true block
    [[nodiscard]] BlockExpression* block_mut() noexcept {
      return gal::as_mut<BlockExpression>(block_.get());
    }

    /// Gets the owner of the block to enter if the condition is true
    ///
    /// \return The owner of the true block
    [[nodiscard]] std::unique_ptr<Expression>* block_owner() noexcept {
      return &block_;
    }

    /// Gets the list of elif blocks
    ///
    /// \return The list of elif bloks
    [[nodiscard]] absl::Span<const ElifBlock> elif_blocks() const noexcept {
      return elif_blocks_;
    }

    /// Gets the list of elif blocks
    ///
    /// \return The list of elif bloks
    [[nodiscard]] absl::Span<ElifBlock> elif_blocks_mut() noexcept {
      return absl::MakeSpan(elif_blocks_);
    }

    /// Gets the else block if it exists
    ///
    /// \return The else block
    [[nodiscard]] std::optional<const BlockExpression*> else_block() const noexcept {
      if (is_evaluable()) {
        return gal::as_mut<const BlockExpression>(else_block_->get());
      }

      return std::nullopt;
    }

    /// Gets the else block if it exists
    ///
    /// \return The else block
    [[nodiscard]] std::optional<BlockExpression*> else_block_mut() noexcept {
      if (is_evaluable()) {
        return gal::as_mut<BlockExpression>(else_block_->get());
      }

      return std::nullopt;
    }

    /// Gets the owner of the else block if it exists
    ///
    /// \return The owner of the else block
    [[nodiscard]] std::optional<std::unique_ptr<Expression>*> else_block_owner() noexcept {
      return is_evaluable() ? std::make_optional(&*else_block_) : std::nullopt;
    }

    /// Checks if the if block is actually able to be evaluated, i.e whether or not
    /// it has an else block (if it doesn't, it may not always yield a value)
    ///
    /// \return Whether or not the statement is able to be evaluated
    [[nodiscard]] bool is_evaluable() const noexcept {
      return else_block_.has_value();
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Expression> block_;
    std::vector<ElifBlock> elif_blocks_;
    std::optional<std::unique_ptr<Expression>> else_block_;
  };

  /// Maps to an unconditional loop, i.e `loop { ... }`
  class LoopExpression final : public Expression {
  public:
    /// Makes a loop expression
    ///
    /// \param loc The location in the source code
    /// \param body The body of the loop
    explicit LoopExpression(SourceLoc loc, std::unique_ptr<Expression> body) noexcept
        : Expression(std::move(loc), ExprType::loop),
          body_{std::move(body)} {};

    /// Gets the body of the loop
    ///
    /// \return The body of the loop
    [[nodiscard]] const BlockExpression& body() const noexcept {
      return gal::as<BlockExpression>(*body_);
    }

    /// Gets the body of the loop
    ///
    /// \return The body of the loop
    [[nodiscard]] BlockExpression* body_mut() noexcept {
      return gal::as_mut<BlockExpression>(body_.get());
    }

    /// Gets the owner of the body of the loop
    ///
    /// \return The owner of the body of the loop
    [[nodiscard]] std::unique_ptr<Expression>* body_owner() noexcept {
      return &body_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> body_;
  };

  /// Models a `while` loop
  class WhileExpression final : public Expression {
  public:
    /// Creates a while loop
    ///
    /// \param loc The location in the source code of the loop
    /// \param condition The condition of the loop
    /// \param body The body of the loop
    explicit WhileExpression(SourceLoc loc,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<BlockExpression> body) noexcept
        : Expression(std::move(loc), ExprType::while_loop),
          condition_{std::move(condition)},
          body_{std::move(body)} {};

    /// Gets the condition of the loop
    ///
    /// \return The condition of the loop
    [[nodiscard]] const Expression& condition() const noexcept {
      return *condition_;
    }

    /// Gets the condition of the loop
    ///
    /// \return The condition of the loop
    [[nodiscard]] Expression* condition_mut() noexcept {
      return condition_.get();
    }

    /// Gets the owner of the condition of the loop
    ///
    /// \return The owner of the condition of the loop
    [[nodiscard]] std::unique_ptr<Expression>* condition_owner() noexcept {
      return &condition_;
    }

    /// Gets the body of the loop
    ///
    /// \return The body of the loop
    [[nodiscard]] const BlockExpression& body() const noexcept {
      return gal::as<BlockExpression>(*body_);
    }

    /// Gets the body of the loop
    ///
    /// \return The body of the loop
    [[nodiscard]] BlockExpression* body_mut() noexcept {
      return gal::as_mut<BlockExpression>(body_.get());
    }

    /// Gets the owner of the body of the loop
    ///
    /// \return The owner of the body of the loop
    [[nodiscard]] std::unique_ptr<Expression>* body_owner() noexcept {
      return &body_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Expression> body_;
  };

  /// The possible directions of the for-loop
  enum class ForDirection {
    up_to,
    down_to,
  };

  /// Models a for loop
  class ForExpression final : public Expression {
  public:
    /// Creates a for-loop
    ///
    /// \param loc The source location
    /// \param loop_variable The name of the loop iterator variable
    /// \param direction The direction of the loop
    /// \param init The initial value of the loop iterator
    /// \param last The value to stop at
    /// \param body The body of the loop
    explicit ForExpression(SourceLoc loc,
        std::string loop_variable,
        ForDirection direction,
        std::unique_ptr<Expression> init,
        std::unique_ptr<Expression> last,
        std::unique_ptr<BlockExpression> body) noexcept
        : Expression(std::move(loc), ExprType::for_loop),
          loop_variable_{std::move(loop_variable)},
          direction_{direction},
          init_{std::move(init)},
          last_{std::move(last)},
          body_{std::move(body)} {}

    /// Gets the name of the loop variable
    ///
    /// \return The name of the loop variable
    [[nodiscard]] std::string_view loop_variable() const noexcept {
      return loop_variable_;
    }

    /// Gets the direction that the loop variable will be incremented/decremented
    ///
    /// \return The direction of the loop
    [[nodiscard]] ForDirection loop_direction() const noexcept {
      return direction_;
    }

    /// Gets the init expression of the loop
    ///
    /// \return The init expression of the loop
    [[nodiscard]] const Expression& init() const noexcept {
      return *init_;
    }

    /// Gets the init expression of the loop
    ///
    /// \return The init expression of the loop
    [[nodiscard]] Expression* init_mut() noexcept {
      return init_.get();
    }

    /// Gets the owner of the init expression of the loop
    ///
    /// \return The owner of the init expression of the loop
    [[nodiscard]] std::unique_ptr<Expression>* init_owner() noexcept {
      return &init_;
    }

    /// Gets the value to stop at
    ///
    /// \return The value to stop at
    [[nodiscard]] const Expression& last() const noexcept {
      return *last_;
    }

    /// Gets the value to stop at
    ///
    /// \return The value to stop at
    [[nodiscard]] Expression* last_mut() noexcept {
      return last_.get();
    }

    /// Gets the owner of the value to stop at
    ///
    /// \return The value to stop at
    [[nodiscard]] std::unique_ptr<Expression>* last_owner() noexcept {
      return &last_;
    }

    /// Gets the body of the loop
    ///
    /// \return The body of the loop
    [[nodiscard]] const BlockExpression& body() const noexcept {
      return gal::as<BlockExpression>(*body_);
    }

    /// Gets the body of the loop
    ///
    /// \return The body of the loop
    [[nodiscard]] BlockExpression* body_mut() noexcept {
      return gal::as_mut<BlockExpression>(body_.get());
    }

    /// Gets the owner of the body of the loop
    ///
    /// \return The owner of the body of the loop
    [[nodiscard]] std::unique_ptr<Expression>* body_owner() noexcept {
      return &body_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::string loop_variable_;
    ForDirection direction_;
    std::unique_ptr<Expression> init_;
    std::unique_ptr<Expression> last_;
    std::unique_ptr<Expression> body_;
  };

  /// Models a return expression
  class ReturnExpression final : public Expression {
  public:
    /// Creates a return expression
    ///
    /// \param loc The location in the source
    /// \param value The value being returned
    explicit ReturnExpression(SourceLoc loc, std::optional<std::unique_ptr<Expression>> value) noexcept
        : Expression(std::move(loc), ExprType::return_expr),
          value_{std::move(value)} {}

    /// Gets the value being returned
    ///
    /// \return The value being returned
    [[nodiscard]] std::optional<const Expression*> value() const noexcept {
      return (value_.has_value()) ? std::make_optional(value_->get()) : std::nullopt;
    }

    /// Gets the value being returned
    ///
    /// \return The value being returned
    [[nodiscard]] std::optional<Expression*> value_mut() noexcept {
      return (value_.has_value()) ? std::make_optional(value_->get()) : std::nullopt;
    }

    /// Gets the owner of the value being returned
    ///
    /// \return The owner of the value being returned
    [[nodiscard]] std::optional<std::unique_ptr<Expression>*> value_owner() noexcept {
      return (value_.has_value()) ? std::make_optional(&*value_) : std::nullopt;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::optional<std::unique_ptr<Expression>> value_;
  };

  /// Models a break expression
  class BreakExpression final : public Expression {
  public:
    /// Creates a break expression
    ///
    /// \param loc The location in the source
    /// \param value The value being returned, if it exists
    explicit BreakExpression(SourceLoc loc, std::optional<std::unique_ptr<Expression>> value) noexcept
        : Expression(std::move(loc), ExprType::break_expr),
          value_{std::move(value)} {}

    /// Gets the value being returned
    ///
    /// \return The value being returned
    [[nodiscard]] std::optional<const Expression*> value() const noexcept {
      return (value_.has_value()) ? std::make_optional(value_->get()) : std::nullopt;
    }

    /// Gets the value being returned
    ///
    /// \return The value being returned
    [[nodiscard]] std::optional<Expression*> value_mut() noexcept {
      return (value_.has_value()) ? std::make_optional(value_->get()) : std::nullopt;
    }

    /// Gets the owner of the value being returned
    ///
    /// \return The owner of the value being returned
    [[nodiscard]] std::optional<std::unique_ptr<Expression>*> value_owner() noexcept {
      return (value_.has_value()) ? std::make_optional(&*value_) : std::nullopt;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::optional<std::unique_ptr<Expression>> value_;
  };

  /// Models a continue expression
  class ContinueExpression final : public Expression {
  public:
    /// Creates a continue expression
    ///
    /// \param loc The location in the source
    explicit ContinueExpression(SourceLoc loc) noexcept : Expression(std::move(loc), ExprType::continue_expr) {}

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;
  };

  /// Models an initializer for a single field, i.e `x: 32.4` in `Point { x: 32.4, y: 0.0 }`
  class FieldInitializer {
  public:
    /// Creates a field initializer
    ///
    /// \param name The name of the field
    /// \param init The initializer for the field
    explicit FieldInitializer(SourceLoc loc, std::string name, std::unique_ptr<Expression> init) noexcept
        : loc_{loc},
          name_{std::move(name)},
          initializer_{std::move(init)} {}

    /// Copies a field initializer
    ///
    /// \param other The field initializer to copy
    FieldInitializer(const FieldInitializer& other) noexcept
        : loc_{other.loc()},
          name_{other.name()},
          initializer_{other.init().clone()} {}

    /// Gets the location in the source of the field init
    ///
    /// \return The field init
    [[nodiscard]] const ast::SourceLoc& loc() const noexcept {
      return loc_;
    }

    /// Gets the name of the field
    ///
    /// \return The name of the field
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// Gets the value to initialize the field to
    ///
    /// \return The initializer
    [[nodiscard]] const Expression& init() const noexcept {
      return *initializer_;
    }

    /// Gets the value to initialize the field to
    ///
    /// \return The initializer
    [[nodiscard]] Expression* init_mut() noexcept {
      return initializer_.get();
    }

    /// Gets the owner of the value to initialize the field to
    ///
    /// \return The owner of the initializer
    [[nodiscard]] std::unique_ptr<Expression>* init_owner() noexcept {
      return &initializer_;
    }

    /// Compares two field initializers
    ///
    /// \param lhs The first field init
    /// \param rhs The second field init
    /// \return Whether they are equal
    friend bool operator==(const FieldInitializer& lhs, const FieldInitializer& rhs) noexcept;

  private:
    SourceLoc loc_;
    std::string name_;
    std::unique_ptr<Expression> initializer_;
  };

  /// Models a struct-init expression
  class StructExpression final : public Expression {
  public:
    /// Creates a struct-init expr
    ///
    /// \param loc The location of the expr
    /// \param struct_type The type of the expr
    /// \param fields The fields being initialized
    explicit StructExpression(SourceLoc loc,
        std::unique_ptr<ast::Type> struct_type,
        std::vector<FieldInitializer> fields) noexcept
        : Expression(std::move(loc), ExprType::struct_expr),
          struct_{std::move(struct_type)},
          fields_{std::move(fields)} {}

    /// Gets the type of the struct being initialized
    ///
    /// \return The type of the struct being initialized
    [[nodiscard]] const Type& struct_type() const noexcept {
      return *struct_;
    }

    /// Gets the type of the struct being initialized
    ///
    /// \return The type of the struct being initialized
    [[nodiscard]] Type* struct_type_mut() noexcept {
      return struct_.get();
    }

    /// Gets the type of the struct being initialized
    ///
    /// \return The type of the struct being initialized
    [[nodiscard]] std::unique_ptr<Type>* struct_type_owner() noexcept {
      return &struct_;
    }

    /// Gets the list of fields being initialized by the expression
    ///
    /// \return The fields being initialized
    [[nodiscard]] absl::Span<const FieldInitializer> fields() const noexcept {
      return fields_;
    }

    /// Gets the list of fields being initialized by the expression
    ///
    /// \return The fields being initialized
    [[nodiscard]] absl::Span<FieldInitializer> fields_mut() noexcept {
      return absl::MakeSpan(fields_);
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<ast::Type> struct_;
    std::vector<FieldInitializer> fields_;
  };

  /// Models an implicit conversion that the compiler generated
  class ImplicitConversionExpression final : public Expression {
  public:
    /// Creates an implicit conversion
    ///
    /// \param expr An expression being converted
    /// \param convert_to The type to convert it to
    explicit ImplicitConversionExpression(std::unique_ptr<Expression> expr, std::unique_ptr<Type> convert_to) noexcept
        : Expression(expr->loc(), ExprType::implicit),
          expr_{std::move(expr)},
          cast_to_{std::move(convert_to)} {}

    /// Gets the expression being converted
    ///
    /// \return The expression being converted
    [[nodiscard]] const ast::Expression& expr() const noexcept {
      return *expr_;
    }

    /// Gets the expression being converted
    ///
    /// \return The expression being converted
    [[nodiscard]] ast::Expression* expr_mut() noexcept {
      return expr_.get();
    }

    /// Gets the expression being converted
    ///
    /// \return The expression being converted
    [[nodiscard]] std::unique_ptr<ast::Expression>* expr_owner() noexcept {
      return &expr_;
    }

    /// Gets the type being converted to
    ///
    /// \return The type being converted to
    [[nodiscard]] const ast::Type& cast_to() const noexcept {
      return *cast_to_;
    }

    /// Gets the type being converted to
    ///
    /// \return The type being converted to
    [[nodiscard]] ast::Type* cast_to_mut() noexcept {
      return cast_to_.get();
    }

    /// Gets the type being converted to
    ///
    /// \return The type being converted to
    [[nodiscard]] std::unique_ptr<ast::Type>* cast_to_owner() noexcept {
      return &cast_to_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> expr_;
    std::unique_ptr<Type> cast_to_;
  };

  class ErrorExpression final : public Expression {
  public:
    explicit ErrorExpression() : Expression(SourceLoc::nonexistent(), ExprType::error_expr) {}

  protected:
    void internal_accept(ExpressionVisitorBase*) final;

    void internal_accept(ConstExpressionVisitorBase*) const final;

    [[nodiscard]] bool internal_equals(const Expression&) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;
  };

  /// Models an array expression
  class ArrayExpression final : public Expression {
  public:
    /// Creates an array expression
    ///
    /// \param loc The location in the source
    /// \param elements The list of elements in the array
    explicit ArrayExpression(ast::SourceLoc loc, std::vector<std::unique_ptr<ast::Expression>> elements) noexcept
        : Expression(std::move(loc), ExprType::array),
          elements_{std::move(elements)} {}

    /// Gets the elements of the array
    ///
    /// \return The elements of the array
    [[nodiscard]] absl::Span<const std::unique_ptr<ast::Expression>> elements() const noexcept {
      return elements_;
    }

    /// Gets the elements of the array
    ///
    /// \return The elements of the array
    [[nodiscard]] absl::Span<std::unique_ptr<ast::Expression>> elements_mut() noexcept {
      return absl::MakeSpan(elements_);
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::vector<std::unique_ptr<ast::Expression>> elements_;
  };

  class LoadExpression final : public Expression {
  public:
    /// Creates a load expression
    ///
    /// \param loc The location in the source
    /// \param loaded_from The expression being loaded from
    explicit LoadExpression(SourceLoc loc, std::unique_ptr<Expression> loaded_from) noexcept
        : Expression(std::move(loc), ExprType::load),
          loaded_from_{std::move(loaded_from)} {}

    /// Gets the expression being loaded from
    ///
    /// \return The expression being loaded from
    [[nodiscard]] const Expression& expr() const noexcept {
      return *loaded_from_;
    }

    /// Gets the expression being loaded from
    ///
    /// \return The expression being loaded from
    [[nodiscard]] Expression* expr_mut() noexcept {
      return loaded_from_.get();
    }

    /// Gets the expression being loaded from
    ///
    /// \return The expression being loaded from
    [[nodiscard]] std::unique_ptr<Expression>* expr_owner() noexcept {
      return &loaded_from_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> loaded_from_;
  };

  class AddressOfExpression final : public Expression {
  public:
    /// Creates an addr-of expression
    ///
    /// \param loc The location in the source
    /// \param loaded_from The expression having its address taken
    explicit AddressOfExpression(SourceLoc loc, std::unique_ptr<Expression> loaded_from) noexcept
        : Expression(std::move(loc), ExprType::address_of),
          loaded_from_{std::move(loaded_from)} {}

    /// Gets the expression that will have its address taken
    ///
    /// \return The expression that will have its address taken
    [[nodiscard]] const Expression& expr() const noexcept {
      return *loaded_from_;
    }

    /// Gets the expression that will have its address taken
    ///
    /// \return The expression that will have its address taken
    [[nodiscard]] Expression* expr_mut() noexcept {
      return loaded_from_.get();
    }

    /// Gets the expression that will have its address taken
    ///
    /// \return The expression that will have its address taken
    [[nodiscard]] std::unique_ptr<Expression>* expr_owner() noexcept {
      return &loaded_from_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Expression> loaded_from_;
  };

  class StaticGlobalExpression final : public Expression {
  public:
    explicit StaticGlobalExpression(SourceLoc loc,
        const Declaration& decl,
        std::vector<std::unique_ptr<Type>> generic_params = {}) noexcept
        : Expression(std::move(loc), ExprType::static_global),
          decl_{decl},
          generic_params_{std::move(generic_params)} {}

    [[nodiscard]] const Declaration& decl() const noexcept {
      return decl_;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    const Declaration& decl_;
    std::vector<std::unique_ptr<Type>> generic_params_;
  };

  class SliceOfExpression final : public Expression {
  public:
    explicit SliceOfExpression(ast::SourceLoc loc,
        std::unique_ptr<ast::Expression> data,
        std::unique_ptr<ast::Expression> size)
        : Expression(std::move(loc), ExprType::slice_of),
          data_{std::move(data)},
          size_{std::move(size)} {}

    [[nodiscard]] const Expression& data() const noexcept {
      return *data_;
    }

    [[nodiscard]] Expression* data_mut() noexcept {
      return data_.get();
    }

    [[nodiscard]] std::unique_ptr<Expression>* data_owner() noexcept {
      return &data_;
    }

    [[nodiscard]] const Expression& size() const noexcept {
      return *size_;
    }

    [[nodiscard]] Expression* size_mut() noexcept {
      return size_.get();
    }

    [[nodiscard]] std::unique_ptr<Expression>* size_owner() noexcept {
      return &size_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<ast::Expression> data_;
    std::unique_ptr<ast::Expression> size_;
  };

  class RangeExpression final : public Expression {
  public:
    explicit RangeExpression(ast::SourceLoc loc,
        std::unique_ptr<Expression> array,
        std::unique_ptr<Expression> begin,
        std::unique_ptr<Expression> end,
        Range range)
        : Expression(std::move(loc), ExprType::range_into),
          array_{std::move(array)},
          begin_{std::move(begin)},
          end_{std::move(end)},
          range_{range} {}

    [[nodiscard]] const Expression& array() const noexcept {
      return *array_;
    }

    [[nodiscard]] Expression* array_mut() noexcept {
      return array_.get();
    }

    [[nodiscard]] std::unique_ptr<Expression>* array_owner() noexcept {
      return &array_;
    }

    [[nodiscard]] const Expression& begin() const noexcept {
      return *begin_;
    }

    [[nodiscard]] Expression* begin_mut() noexcept {
      return begin_.get();
    }

    [[nodiscard]] std::unique_ptr<Expression>* begin_owner() noexcept {
      return &begin_;
    }

    [[nodiscard]] const Expression& end() const noexcept {
      return *end_;
    }

    [[nodiscard]] Expression* end_mut() noexcept {
      return end_.get();
    }

    [[nodiscard]] std::unique_ptr<Expression>* end_owner() noexcept {
      return &end_;
    }

    [[nodiscard]] Range range() const noexcept {
      return range_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<ast::Expression> array_;
    std::unique_ptr<ast::Expression> begin_;
    std::unique_ptr<ast::Expression> end_;
    Range range_;
  };

  class SizeofExpression final : public Expression {
  public:
    explicit SizeofExpression(ast::SourceLoc loc, std::unique_ptr<ast::Type> type)
        : Expression(std::move(loc), ExprType::sizeof_type),
          type_{std::move(type)} {}

    [[nodiscard]] const Type& to_check() const noexcept {
      return *type_;
    }

    [[nodiscard]] Type* to_check_mut() noexcept {
      return type_.get();
    }

    [[nodiscard]] std::unique_ptr<Type>* to_check_owner() noexcept {
      return &type_;
    }

  protected:
    void internal_accept(ExpressionVisitorBase* visitor) final;

    void internal_accept(ConstExpressionVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Expression& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Expression> internal_clone() const noexcept final;

  private:
    std::unique_ptr<ast::Type> type_;
  };
} // namespace gal::ast
