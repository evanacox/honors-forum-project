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
  class Statement {
  public:
    Statement() = delete;

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
    virtual void accept(StatementVisitorBase* visitor) = 0;

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void accept(const ConstStatementVisitorBase& visitor) const = 0;

    /// Helper that allows a visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(StatementVisitor<T>* visitor) {
      accept(static_cast<StatementVisitorBase*>(visitor));

      return visitor->move_result();
    }

    /// Helper that allows a const visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(const ConstStatementVisitor<T>& visitor) {
      accept(static_cast<const ConstStatementVisitorBase&>(visitor));

      return visitor->move_result();
    }

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Statement() = default;

  protected:
    /// Initializes the state of the Statement base class
    ///
    /// \param exported Whether or not this particular Statement is marked `export`
    explicit Statement(StmtType real) noexcept : real_{real} {}

    /// Protected so only derived can copy
    Statement(const Statement&) = default;

    /// Protected so only derived can move
    Statement(Statement&&) = default;

  private:
    StmtType real_;
  };

  /// Represents a "binding", i.e `let x = 5` or `var s = String("Hello")`
  class BindingStatement : public Statement {
  public:
    /// Initializes a Binding statement
    ///
    /// \param name The name given to the binding
    /// \param initializer The initializer expression
    /// \param hint The type hint, i.e the `: T` part in `let x: T = foo`
    explicit BindingStatement(std::string name,
        std::unique_ptr<Expression> initializer,
        std::unique_ptr<Type> hint = nullptr) noexcept
        : Statement(StmtType::binding),
          name_{std::move(name)},
          initializer_{std::move(initializer)},
          hint_{std::move(hint)} {}

    ///
    ///
    /// \return
    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    ///
    ///
    /// \return
    [[nodiscard]] const Expression& initializer() const noexcept {
      return *initializer_;
    }

    ///
    ///
    /// \return
    [[nodiscard]] Expression* initializer() noexcept {
      return initializer_.get();
    }

    ///
    ///
    /// \return
    [[nodiscard]] bool has_hint() const noexcept {
      return hint_ != nullptr;
    }

    ///
    ///
    /// \return
    [[nodiscard]] const Type& hint() const noexcept {
      assert(has_hint());

      return *hint_;
    }

  private:
    std::string name_;
    std::unique_ptr<Expression> initializer_;
    std::unique_ptr<Type> hint_;
  };

  ///
  class AssertStatement : public Statement {
  public:
    ///
    ///
    /// \param assertion
    /// \param message
    explicit AssertStatement(std::unique_ptr<Expression> assertion,
        std::unique_ptr<StringLiteralExpression> message) noexcept
        : Statement(StmtType::assertion),
          assertion_{std::move(assertion)},
          message_{std::move(message)} {}

    ///
    ///
    /// \return
    [[nodiscard]] const Expression& assertion() const noexcept {
      return *assertion_;
    }

    ///
    ///
    /// \return
    [[nodiscard]] const StringLiteralExpression& message() const noexcept {
      return *message_;
    }

  private:
    std::unique_ptr<Expression> assertion_;
    std::unique_ptr<StringLiteralExpression> message_;
  };
} // namespace gal::ast
