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

#include "./node.h"
#include "./statement_visitor.h"
#include "./type.h"
#include <string>

namespace gal::ast {
  enum class StmtType {
    binding,
    assertion,
    expr,
  };

  /// Abstract base type for all "Statement" AST nodes
  ///
  /// Is able to be visited by a `StatementVisitorBase`, and can be queried
  /// on whether it's exported and what real type of Statement it is
  class Statement : public Node {
  public:
    Statement() = delete;

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Statement() = default;

    /// Gets the real statement type that the statement actually is
    ///
    /// \return The type of the Statement
    [[nodiscard]] StmtType type() const noexcept {
      return real_;
    }

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    void accept(StatementVisitorBase* visitor) {
      internal_accept(visitor);
    }

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    void accept(ConstStatementVisitorBase* visitor) const {
      internal_accept(visitor);
    }

    /// Helper that allows a visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(StatementVisitor<T>* visitor) {
      accept(static_cast<StatementVisitorBase*>(visitor));

      return visitor->take_result();
    }

    /// Helper that allows a const visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(ConstStatementVisitor<T>* visitor) const {
      accept(static_cast<ConstStatementVisitorBase*>(visitor));

      return visitor->take_result();
    }

    /// Compares two types for equality
    ///
    /// \param other The other type node to compare
    /// \return Whether the two types are equal
    [[nodiscard]] friend bool operator==(const Statement& lhs, const Statement& rhs) noexcept {
      if (lhs.type() == rhs.type()) {
        return lhs.internal_equals(rhs);
      }

      return false;
    }

    /// Compares two type nodes for complete equality, including source location.
    /// Equivalent to `a == b && a.loc() == b.loc()`
    ///
    /// \param rhs The other node to compare
    /// \return Whether the nodes are identical in every observable way
    [[nodiscard]] bool fully_equals(const Statement& rhs) noexcept {
      return *this == rhs && loc() == rhs.loc();
    }

    /// Clones the node and returns a `unique_ptr` to the copy of the node
    ///
    /// \return A new node with the same observable state
    [[nodiscard]] std::unique_ptr<Statement> clone() const noexcept {
      return internal_clone();
    }

    /// Checks if a node is of a particular type in slightly
    /// nicer form than `.type() ==`
    ///
    /// \param type The type to compare against
    /// \return Whether or not the node is of that type
    [[nodiscard]] bool is(StmtType type) const noexcept {
      return real_ == type;
    }

    /// Checks if a node is one of a set of types
    ///
    /// \tparam Args The list of types
    /// \param types The types to check against
    /// \return Whether or not the node is of one of those types
    template <typename... Args> [[nodiscard]] bool is_one_of(Args... types) const noexcept {
      return (is(types) && ...);
    }

  protected:
    /// Initializes the state of the Statement base class
    ///
    /// \param exported Whether or not this particular Statement is marked `export`
    explicit Statement(SourceLoc loc, StmtType real) noexcept : Node(std::move(loc)), real_{real} {}

    /// Protected so only derived can copy
    Statement(const Statement&) = default;

    /// Protected so only derived can move
    Statement(Statement&&) = default;

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void internal_accept(StatementVisitorBase* visitor) = 0;

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void internal_accept(ConstStatementVisitorBase* visitor) const = 0;

    /// Compares two statement objects of the same underlying type for equality
    ///
    /// \param other The other node to compare
    /// \return Whether the nodes are equal
    [[nodiscard]] virtual bool internal_equals(const Statement& other) const noexcept = 0;

    /// Creates a clone of the node where `clone()` is called on
    ///
    /// \return A node equal in all observable ways
    [[nodiscard]] virtual std::unique_ptr<Statement> internal_clone() const noexcept = 0;

  private:
    StmtType real_;
  };

  /// Represents a "binding", i.e `let x = 5` or `var s = String("Hello")`
  class BindingStatement final : public Statement {
  public:
    /// Initializes a Binding statement
    ///
    /// \param name The name given to the binding
    /// \param initializer The initializer expression
    /// \param hint The type hint, i.e the `: T` part in `let x: T = foo`
    explicit BindingStatement(SourceLoc loc,
        std::string name,
        std::unique_ptr<Expression> initializer,
        std::optional<std::unique_ptr<Type>> hint) noexcept
        : Statement(std::move(loc), StmtType::binding),
          name_{std::move(name)},
          initializer_{std::move(initializer)},
          hint_{std::move(hint)} {}

    /// Gets the name of the binding
    ///
    /// \return The name given to the value
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    /// The value given to the binding as the initializer
    ///
    /// \return The initializer
    [[nodiscard]] const Expression& initializer() const noexcept {
      return *initializer_;
    }

    /// The value given to the binding as the initializer
    ///
    /// \return A mutable reference to the initializer
    [[nodiscard]] Expression* initializer_mut() noexcept {
      return initializer_.get();
    }

    /// Gets the type hint for the binding. If it exists, a valid pointer is
    /// returned. If it doesn't, a nullopt is returned
    ///
    /// \return A possible type hint
    [[nodiscard]] std::optional<const Type*> hint() const noexcept {
      return (hint_.has_value()) ? std::make_optional(hint_->get()) : std::nullopt;
    }

    /// Gets the type hint for the binding. If it exists, a valid pointer is
    /// returned. If it doesn't, a nullopt is returned
    ///
    /// \return A possible type hint
    [[nodiscard]] std::optional<Type*> hint_mut() noexcept {
      return (hint_.has_value()) ? std::make_optional(hint_->get()) : std::nullopt;
    }

    /// If the hint exists, returns a pointer to the hint's owner
    /// Otherwise returns a nullopt
    ///
    /// \return A possible type hint
    [[nodiscard]] std::optional<std::unique_ptr<Type>*> hint_owner() noexcept {
      return (hint_.has_value()) ? std::make_optional(&*hint_) : std::nullopt;
    }

  protected:
    void internal_accept(StatementVisitorBase* visitor) final {
      visitor->visit(this);
    }

    void internal_accept(ConstStatementVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

    [[nodiscard]] bool internal_equals(const Statement& other) const noexcept final {
      auto& result = internal::debug_cast<const BindingStatement&>(other);

      return name() == result.name() && gal::unwrapping_equal(hint(), result.hint(), gal::DerefEq{})
             && initializer() == result.initializer();
    }

    [[nodiscard]] std::unique_ptr<Statement> internal_clone() const noexcept final {
      return std::make_unique<BindingStatement>(loc(), name_, initializer().clone(), gal::clone_if(hint_));
    }

  private:
    std::string name_;
    std::unique_ptr<Expression> initializer_;
    std::optional<std::unique_ptr<Type>> hint_;
  };

  /// Models an assertion statement in the code
  class AssertStatement final : public Statement {
  public:
    /// Creates an assertion
    ///
    /// \param assertion The expression to insert
    /// \param message The message to display if the assertion fires
    explicit AssertStatement(SourceLoc loc,
        std::unique_ptr<Expression> assertion,
        std::unique_ptr<StringLiteralExpression> message) noexcept
        : Statement(std::move(loc), StmtType::assertion),
          assertion_{std::move(assertion)},
          message_{std::move(message)} {}

    /// Gets the assertion that needs to be checked
    ///
    /// \return The assertion
    [[nodiscard]] const Expression& assertion() const noexcept {
      return *assertion_;
    }

    /// Gets the assertion that needs to be checked
    ///
    /// \return The assertion
    [[nodiscard]] Expression* assertion_mut() noexcept {
      return assertion_.get();
    }

    /// Gets the assertion that needs to be checked
    ///
    /// \return The assertion
    [[nodiscard]] std::unique_ptr<Expression>* assertion_owner() noexcept {
      return &assertion_;
    }

    /// Gets the message that the assertion was given
    ///
    /// \return The assertion message
    [[nodiscard]] const StringLiteralExpression& message() const noexcept {
      return *message_;
    }

  protected:
    void internal_accept(StatementVisitorBase* visitor) final {
      visitor->visit(this);
    }

    void internal_accept(ConstStatementVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

    [[nodiscard]] bool internal_equals(const Statement& other) const noexcept final {
      auto& result = internal::debug_cast<const AssertStatement&>(other);

      return assertion() == result.assertion() && message() == result.message();
    }

    [[nodiscard]] std::unique_ptr<Statement> internal_clone() const noexcept final {
      return std::make_unique<AssertStatement>(loc(),
          assertion().clone(),
          gal::static_unique_cast<StringLiteralExpression>(message().clone()));
    }

  private:
    std::unique_ptr<Expression> assertion_;
    std::unique_ptr<StringLiteralExpression> message_;
  };

  class ExpressionStatement final : public Statement {
  public:
    /// Creates an expression statement
    ///
    /// \param loc The location in the source code
    /// \param expr The expression
    explicit ExpressionStatement(SourceLoc loc, std::unique_ptr<Expression> expr) noexcept
        : Statement(std::move(loc), StmtType::expr),
          expr_{std::move(expr)} {}

    /// Gets the expression in the statement
    ///
    /// \return The expression
    [[nodiscard]] const Expression& expr() const noexcept {
      return *expr_;
    }

    /// Gets the expression in the statement
    ///
    /// \return The expression
    [[nodiscard]] Expression* expr_mut() noexcept {
      return expr_.get();
    }

    /// Gets the expression in the statement
    ///
    /// \return The expression
    [[nodiscard]] std::unique_ptr<Expression>* expr_owner() noexcept {
      return &expr_;
    }

  protected:
    void internal_accept(StatementVisitorBase* visitor) final {
      visitor->visit(this);
    }

    void internal_accept(ConstStatementVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

    [[nodiscard]] bool internal_equals(const Statement& other) const noexcept final {
      auto& result = internal::debug_cast<const ExpressionStatement&>(other);

      return expr() == result.expr();
    }

    [[nodiscard]] std::unique_ptr<Statement> internal_clone() const noexcept final {
      return std::make_unique<ExpressionStatement>(loc(), expr().clone());
    }

  private:
    std::unique_ptr<Expression> expr_;
  };
} // namespace gal::ast
