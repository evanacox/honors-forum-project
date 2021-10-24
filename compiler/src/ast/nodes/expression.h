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

#include "./expression_visitor.h"
#include "./modular_id.h"
#include "./node.h"
#include "./type.h"
#include "absl/types/span.h"
#include <limits>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace gal::ast {
  class Statement;

  enum class ExprType {
    string_lit,
    integer_lit,
    float_lit,
    bool_lit,
    char_lit,
    nil_lit,
    group,
    identifier,
    block,
    call,
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
    eq_eq,
    bang_eq,
    left_shift,
    right_shift,
    bitwise_and,
    bitwise_or,
    bitwise_xor,
    logical_and,
    logical_or,
    logical_xor,
    walrus,
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

    /// Gets the real underlying type of the expression
    ///
    /// \return The type of the expression
    [[nodiscard]] ExprType type() const noexcept {
      return real_;
    }

    /// Checks if `.result()` is safe to call
    ///
    /// \return
    [[nodiscard]] bool has_result() const noexcept {
      return evaluates_to_ != nullptr;
    }

    /// Gets the result type of the expression, i.e the type it will evaluate to
    ///
    /// \return The type that the expression evaluates to
    [[nodiscard]] const Type& result() const noexcept {
      assert(has_result());

      return *evaluates_to_;
    }

    /// Gets a mutable pointer to the result type
    ///
    /// \return Mutable pointer to the result type
    [[nodiscard]] Type* result_mut() noexcept {
      assert(has_result());

      return evaluates_to_.get();
    }

    /// Exchanges `new_result` and `result()`, returning the old value of `result()`
    ///
    /// \param new_result The new type for the expression to evaluate to
    /// \return The old type
    [[nodiscard]] std::unique_ptr<Type> exchange_result(std::unique_ptr<Type> new_result) noexcept {
      return std::exchange(evaluates_to_, std::move(new_result));
    }

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void accept(ExpressionVisitorBase* visitor) = 0;

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void accept(ConstExpressionVisitorBase* visitor) const = 0;

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

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Expression() = default;

  protected:
    /// Initializes the base type for `Expression`
    ///
    /// \param real The real expression type
    /// \param evaluates_to The type that the expression evaluates to
    explicit Expression(SourceLoc loc, ExprType real, std::unique_ptr<Type> evaluates_to = nullptr)
        : Node(std::move(loc)),
          real_{real},
          evaluates_to_{std::move(evaluates_to)} {}

  private:
    ExprType real_;
    std::unique_ptr<Type> evaluates_to_;
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

      return s.substr(1, s.size() - 1);
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

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

    [[nodiscard]] std::uint64_t value() const noexcept {
      return literal_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::uint64_t literal_;
  };

  class FloatLiteralExpression final : public Expression {
  public:
    static_assert(std::numeric_limits<double>::is_iec559);

    explicit FloatLiteralExpression(SourceLoc loc, double lit) noexcept
        : Expression(std::move(loc), ExprType::float_lit),
          literal_{lit} {}

    [[nodiscard]] double value() const noexcept {
      return literal_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    double literal_;
  };

  class BoolLiteralExpression final : public Expression {
  public:
    explicit BoolLiteralExpression(SourceLoc loc, bool value) noexcept
        : Expression(std::move(loc), ExprType::bool_lit),
          literal_{value} {}

    [[nodiscard]] bool value() const noexcept {
      return literal_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    bool literal_;
  };

  class CharLiteralExpression final : public Expression {
  public:
    explicit CharLiteralExpression(SourceLoc loc, std::uint8_t value) noexcept
        : Expression(std::move(loc), ExprType::char_lit),
          literal_{value} {}

    [[nodiscard]] std::uint8_t value() const noexcept {
      return literal_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::uint8_t literal_;
  };

  class NilLiteral final : public Expression {
  public:
    explicit NilLiteral(SourceLoc loc) noexcept : Expression(std::move(loc), ExprType::nil_lit) {}

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }
  };

  class IdentifierExpression final : public Expression {
  public:
    struct Unqualified {
      std::optional<ModuleID> module_prefix;
      std::string name;
    };

    explicit IdentifierExpression(SourceLoc loc, std::optional<ModuleID> prefix, std::string name) noexcept
        : Expression(std::move(loc), ExprType::identifier),
          state_{Unqualified{std::move(prefix), std::move(name)}} {}

    [[nodiscard]] bool is_qualified() const noexcept {
      return std::holds_alternative<FullyQualifiedID>(state_);
    }

    [[nodiscard]] const Unqualified& unqualified() const noexcept {
      assert(!is_qualified());

      return std::get<Unqualified>(state_);
    }

    [[nodiscard]] Unqualified* unqualified_mut() noexcept {
      assert(!is_qualified());

      return &std::get<Unqualified>(state_);
    }

    void qualify(FullyQualifiedID id) noexcept {
      assert(!is_qualified());

      state_ = std::move(id);
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::variant<Unqualified, FullyQualifiedID> state_;
  };

  class CallExpression final : public Expression {
  public:
    explicit CallExpression(SourceLoc loc,
        std::unique_ptr<Expression> callee,
        std::vector<std::unique_ptr<Expression>> args)
        : Expression(std::move(loc), ExprType::call),
          callee_{std::move(callee)},
          args_{std::move(args)} {}

    [[nodiscard]] const Expression& callee() const noexcept {
      return *callee_;
    }

    [[nodiscard]] Expression* callee() noexcept {
      return callee_.get();
    }

    std::unique_ptr<Expression> exchange_callee(std::unique_ptr<Expression> new_callee) noexcept {
      return std::exchange(callee_, std::move(new_callee));
    }

    [[nodiscard]] absl::Span<const std::unique_ptr<Expression>> args() const noexcept {
      return args_;
    }

    [[nodiscard]] std::vector<std::unique_ptr<Expression>>* args_mut() noexcept {
      return &args_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> callee_;
    std::vector<std::unique_ptr<Expression>> args_;
  };

  class IndexExpression final : public Expression {
  public:
    explicit IndexExpression(SourceLoc loc,
        std::unique_ptr<Expression> callee,
        std::vector<std::unique_ptr<Expression>> args)
        : Expression(std::move(loc), ExprType::index),
          callee_{std::move(callee)},
          args_{std::move(args)} {}

    [[nodiscard]] const Expression& callee() const noexcept {
      return *callee_;
    }

    [[nodiscard]] Expression* callee() noexcept {
      return callee_.get();
    }

    std::unique_ptr<Expression> exchange_callee(std::unique_ptr<Expression> new_callee) noexcept {
      return std::exchange(callee_, std::move(new_callee));
    }

    [[nodiscard]] absl::Span<const std::unique_ptr<Expression>> args() const noexcept {
      return args_;
    }

    [[nodiscard]] std::vector<std::unique_ptr<Expression>>* args_mut() noexcept {
      return &args_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> callee_;
    std::vector<std::unique_ptr<Expression>> args_;
  };

  class FieldAccessExpression final : public Expression {
  public:
    explicit FieldAccessExpression(SourceLoc loc, std::unique_ptr<Expression> object, std::string field) noexcept
        : Expression(std::move(loc), ExprType::field_access),
          object_{std::move(object)},
          field_{std::move(field)} {}

    [[nodiscard]] const Expression& object() const noexcept {
      return *object_;
    }

    [[nodiscard]] Expression* object_mut() noexcept {
      return object_.get();
    }

    std::unique_ptr<Expression> exchange_object(std::unique_ptr<Expression> new_object) noexcept {
      return std::exchange(object_, std::move(new_object));
    }

    [[nodiscard]] std::string_view field_name() const noexcept {
      return field_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> object_;
    std::string field_;
  };

  class GroupExpression final : public Expression {
  public:
    explicit GroupExpression(SourceLoc loc, std::unique_ptr<Expression> grouped) noexcept
        : Expression(std::move(loc), ExprType::group),
          grouped_{std::move(grouped)} {}

    [[nodiscard]] const Expression& expr() const noexcept {
      return *grouped_;
    }

    [[nodiscard]] Expression* expr_mut() noexcept {
      return grouped_.get();
    }

    std::unique_ptr<Expression> exchange_expr(std::unique_ptr<Expression> new_expr) noexcept {
      return std::exchange(grouped_, std::move(new_expr));
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> grouped_;
  };

  class UnaryExpression final : public Expression {
  public:
    explicit UnaryExpression(SourceLoc loc, UnaryOp op, std::unique_ptr<Expression> expr)
        : Expression(std::move(loc), ExprType::unary),
          expr_{std::move(expr)},
          op_{op} {}

    [[nodiscard]] const Expression& expr() const noexcept {
      return *expr_;
    }

    [[nodiscard]] Expression* expr_mut() noexcept {
      return expr_.get();
    }

    std::unique_ptr<Expression> exchange_expr(std::unique_ptr<Expression> new_expr) noexcept {
      return std::exchange(expr_, std::move(new_expr));
    }

    [[nodiscard]] UnaryOp op() const noexcept {
      return op_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> expr_;
    UnaryOp op_;
  };

  class BinaryExpression final : public Expression {
  public:
    explicit BinaryExpression(SourceLoc loc,
        BinaryOp op,
        std::unique_ptr<Expression> lhs,
        std::unique_ptr<Expression> rhs)
        : Expression(std::move(loc), ExprType::binary),
          lhs_{std::move(lhs)},
          rhs_{std::move(rhs)},
          op_{op} {}

    [[nodiscard]] const Expression& lhs() const noexcept {
      return *lhs_;
    }

    [[nodiscard]] Expression* lhs_mut() noexcept {
      return lhs_.get();
    }

    std::unique_ptr<Expression> exchange_lhs(std::unique_ptr<Expression> new_lhs) noexcept {
      return std::exchange(lhs_, std::move(new_lhs));
    }

    [[nodiscard]] const Expression& rhs() const noexcept {
      return *rhs_;
    }

    [[nodiscard]] Expression* rhs_mut() noexcept {
      return rhs_.get();
    }

    std::unique_ptr<Expression> exchange_rhs(std::unique_ptr<Expression> new_rhs) noexcept {
      return std::exchange(rhs_, std::move(new_rhs));
    }

    [[nodiscard]] BinaryOp op() const noexcept {
      return op_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> lhs_;
    std::unique_ptr<Expression> rhs_;
    BinaryOp op_;
  };

  class CastExpression final : public Expression {
  public:
    explicit CastExpression(SourceLoc loc,
        bool unsafe,
        std::unique_ptr<Expression> castee,
        std::unique_ptr<Type> cast_to) noexcept
        : Expression(std::move(loc), ExprType::cast),
          unsafe_{unsafe},
          castee_{std::move(castee)},
          cast_to_{std::move(cast_to)} {}

    [[nodiscard]] bool unsafe() const noexcept {
      return unsafe_;
    }

    [[nodiscard]] const Expression& castee() const noexcept {
      return *castee_;
    }

    [[nodiscard]] Expression* castee_mut() noexcept {
      return castee_.get();
    }

    std::unique_ptr<Expression> exchange_castee(std::unique_ptr<Expression> new_castee) noexcept {
      return std::exchange(castee_, std::move(new_castee));
    }

    [[nodiscard]] const Type& cast_to() const noexcept {
      return *cast_to_;
    }

    std::unique_ptr<Type> exchange_cast_to(std::unique_ptr<Type> new_cast_to) noexcept {
      return std::exchange(cast_to_, std::move(new_cast_to));
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    bool unsafe_;
    std::unique_ptr<Expression> castee_;
    std::unique_ptr<Type> cast_to_;
  };

  class IfThenExpression final : public Expression {
  public:
    explicit IfThenExpression(SourceLoc loc,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<Expression> true_branch,
        std::unique_ptr<Expression> false_branch) noexcept
        : Expression(std::move(loc), ExprType::if_then),
          condition_{std::move(condition)},
          true_branch_{std::move(true_branch)},
          false_branch_{std::move(false_branch)} {}

    [[nodiscard]] const Expression& condition() const noexcept {
      return *condition_;
    }

    [[nodiscard]] Expression* condition_mut() noexcept {
      return condition_.get();
    }

    std::unique_ptr<Expression> exchange_condition(std::unique_ptr<Expression> new_condition) noexcept {
      return std::exchange(condition_, std::move(new_condition));
    }

    [[nodiscard]] const Expression& true_branch() const noexcept {
      return *true_branch_;
    }

    [[nodiscard]] Expression* true_branch_mut() noexcept {
      return true_branch_.get();
    }

    std::unique_ptr<Expression> exchange_true_branch(std::unique_ptr<Expression> new_true_branch) noexcept {
      return std::exchange(true_branch_, std::move(new_true_branch));
    }

    [[nodiscard]] const Expression& false_branch() const noexcept {
      return *false_branch_;
    }

    [[nodiscard]] Expression* false_branch_mut() noexcept {
      return false_branch_.get();
    }

    std::unique_ptr<Expression> exchange_false_branch(std::unique_ptr<Expression> new_false_branch) noexcept {
      return std::exchange(false_branch_, std::move(new_false_branch));
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<Expression> true_branch_;
    std::unique_ptr<Expression> false_branch_;
  };

  class BlockExpression final : public Expression {
  public:
    explicit BlockExpression(SourceLoc loc, std::vector<std::unique_ptr<Statement>> statements) noexcept
        : Expression(std::move(loc), ExprType::block),
          statements_{std::move(statements)} {}

    [[nodiscard]] absl::Span<const std::unique_ptr<Statement>> statements() const noexcept {
      return statements_;
    }

    [[nodiscard]] std::vector<std::unique_ptr<Statement>>* statements_mut() noexcept {
      return &statements_;
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::vector<std::unique_ptr<Statement>> statements_;
  };

  class IfElseExpression final : public Expression {
  public:
    explicit IfElseExpression(SourceLoc loc,
        std::unique_ptr<BlockExpression> block,
        std::unique_ptr<Expression> else_block) noexcept
        : Expression(std::move(loc), ExprType::if_else),
          block_{std::move(block)},
          else_block_{std::move(else_block)} {}

    [[nodiscard]] const BlockExpression& block() const noexcept {
      return *block_;
    }

    [[nodiscard]] BlockExpression* block_mut() noexcept {
      return block_.get();
    }

    std::unique_ptr<BlockExpression> exchange_block(std::unique_ptr<BlockExpression> new_block) noexcept {
      return std::exchange(block_, std::move(new_block));
    }

    [[nodiscard]] const Expression& else_block() const noexcept {
      return *else_block_;
    }

    [[nodiscard]] Expression* else_block_mut() noexcept {
      return else_block_.get();
    }

    std::unique_ptr<Expression> exchange_else_block(std::unique_ptr<Expression> new_else_block) noexcept {
      assert(new_else_block->type() == ExprType::block || new_else_block->type() == ExprType::if_else);

      return std::exchange(else_block_, std::move(new_else_block));
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<BlockExpression> block_;
    std::unique_ptr<Expression> else_block_; // either `BlockExpression` or `IfElseExpression`
  };

  class LoopExpression final : public Expression {
  public:
    explicit LoopExpression(SourceLoc loc, std::unique_ptr<BlockExpression> body) noexcept
        : Expression(std::move(loc), ExprType::loop),
          body_{std::move(body)} {};

    [[nodiscard]] const BlockExpression& body() const noexcept {
      return *body_;
    }

    [[nodiscard]] BlockExpression* body_mut() noexcept {
      return body_.get();
    }

    std::unique_ptr<BlockExpression> exchange_body(std::unique_ptr<BlockExpression> new_body) noexcept {
      return std::exchange(body_, std::move(new_body));
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<BlockExpression> body_;
  };

  class WhileExpression final : public Expression {
  public:
    explicit WhileExpression(SourceLoc loc,
        std::unique_ptr<Expression> condition,
        std::unique_ptr<BlockExpression> body) noexcept
        : Expression(std::move(loc), ExprType::while_loop),
          condition_{std::move(condition)},
          body_{std::move(body)} {};

    [[nodiscard]] const Expression& condition() const noexcept {
      return *condition_;
    }

    [[nodiscard]] Expression* condition_mut() noexcept {
      return condition_.get();
    }

    std::unique_ptr<Expression> exchange_condition(std::unique_ptr<Expression> new_condition) noexcept {
      return std::exchange(condition_, std::move(new_condition));
    }

    [[nodiscard]] const BlockExpression& body() const noexcept {
      return *body_;
    }

    [[nodiscard]] BlockExpression* body_mut() noexcept {
      return body_.get();
    }

    std::unique_ptr<BlockExpression> exchange_body(std::unique_ptr<BlockExpression> new_body) noexcept {
      return std::exchange(body_, std::move(new_body));
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> condition_;
    std::unique_ptr<BlockExpression> body_;
  };

  class ForExpression final : public Expression {
  public:
    enum class Direction {
      up_to,
      down_to,
    };

    explicit ForExpression(SourceLoc loc,
        std::string loop_variable,
        Direction direction,
        std::unique_ptr<Expression> init,
        std::unique_ptr<Expression> last,
        std::unique_ptr<BlockExpression> body) noexcept
        : Expression(std::move(loc), ExprType::for_loop),
          loop_variable_{std::move(loop_variable)},
          direction_{direction},
          init_{std::move(init)},
          last_{std::move(last)},
          body_{std::move(body)} {}

    [[nodiscard]] std::string_view loop_variable() const noexcept {
      return loop_variable_;
    }

    [[nodiscard]] Direction loop_direction() const noexcept {
      return direction_;
    }

    [[nodiscard]] const Expression& init() const noexcept {
      return *init_;
    }

    [[nodiscard]] Expression* init_mut() noexcept {
      return init_.get();
    }

    std::unique_ptr<Expression> exchange_init(std::unique_ptr<Expression> new_init) noexcept {
      return std::exchange(init_, std::move(new_init));
    }

    [[nodiscard]] const Expression& last() const noexcept {
      return *last_;
    }

    [[nodiscard]] Expression* last_mut() noexcept {
      return last_.get();
    }

    std::unique_ptr<Expression> exchange_last(std::unique_ptr<Expression> new_last) noexcept {
      return std::exchange(last_, std::move(new_last));
    }

    [[nodiscard]] const BlockExpression& body() const noexcept {
      return *body_;
    }

    [[nodiscard]] BlockExpression* body_mut() noexcept {
      return body_.get();
    }

    std::unique_ptr<BlockExpression> exchange_body(std::unique_ptr<BlockExpression> new_body) noexcept {
      return std::exchange(body_, std::move(new_body));
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::string loop_variable_;
    Direction direction_;
    std::unique_ptr<Expression> init_;
    std::unique_ptr<Expression> last_;
    std::unique_ptr<BlockExpression> body_;
  };

  class ReturnExpression final : public Expression {
  public:
    explicit ReturnExpression(SourceLoc loc, std::unique_ptr<Expression> value) noexcept
        : Expression(std::move(loc), ExprType::return_expr),
          value_{std::move(value)} {}

    [[nodiscard]] const Expression& value() const noexcept {
      return *value_;
    }

    [[nodiscard]] Expression* value_mut() noexcept {
      return value_.get();
    }

    std::unique_ptr<Expression> exchange_value(std::unique_ptr<Expression> new_value) noexcept {
      return std::exchange(value_, std::move(new_value));
    }

    /// Accepts a visitor that's able to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ExpressionVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the expression
    ///
    /// \param visitor The visitor
    void accept(ConstExpressionVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Expression> value_;
  };
} // namespace gal::ast
