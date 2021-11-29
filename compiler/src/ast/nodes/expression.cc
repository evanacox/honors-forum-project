//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./expression.h"
#include "../../core/environment.h"
#include "./statement.h"

namespace gal::ast {
  void StringLiteralExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void StringLiteralExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  [[nodiscard]] bool StringLiteralExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const StringLiteralExpression&>(other);

    return text() == result.text();
  }

  [[nodiscard]] std::unique_ptr<Expression> StringLiteralExpression::internal_clone() const noexcept {
    return std::make_unique<StringLiteralExpression>(loc(), std::string{text()});
  }

  void IntegerLiteralExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void IntegerLiteralExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool IntegerLiteralExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const IntegerLiteralExpression&>(other);

    return value() == result.value();
  }

  std::unique_ptr<Expression> IntegerLiteralExpression::internal_clone() const noexcept {
    return std::make_unique<IntegerLiteralExpression>(loc(), value());
  }

  void FloatLiteralExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void FloatLiteralExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool FloatLiteralExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const FloatLiteralExpression&>(other);

    // TODO: is this reasonable? should there be an epsilon of some sort?
    gal::outs() << "FloatLiteralExpression::internal_equals: epsilon???";

    return value() == result.value() && str_len() == result.str_len();
  }

  std::unique_ptr<Expression> FloatLiteralExpression::internal_clone() const noexcept {
    return std::make_unique<FloatLiteralExpression>(loc(), value(), str_len());
  }

  void BoolLiteralExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BoolLiteralExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BoolLiteralExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const BoolLiteralExpression&>(other);

    return value() == result.value();
  }

  std::unique_ptr<Expression> BoolLiteralExpression::internal_clone() const noexcept {
    return std::make_unique<BoolLiteralExpression>(loc(), value());
  }

  void CharLiteralExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void CharLiteralExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool CharLiteralExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const CharLiteralExpression&>(other);

    return value() == result.value();
  }

  std::unique_ptr<Expression> CharLiteralExpression::internal_clone() const noexcept {
    return std::make_unique<CharLiteralExpression>(loc(), value());
  }

  void NilLiteralExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void NilLiteralExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool NilLiteralExpression::internal_equals(const Expression& other) const noexcept {
    // doing this for the side effect that it checks the validity in debug mode
    (void)internal::debug_cast<const NilLiteralExpression&>(other);

    return true;
  }

  std::unique_ptr<Expression> NilLiteralExpression::internal_clone() const noexcept {
    return std::make_unique<NilLiteralExpression>(loc());
  }

  bool operator==(const NestedGenericID& lhs, const NestedGenericID& rhs) noexcept {
    return lhs.name == rhs.name
           && std::equal(lhs.ids.begin(), lhs.ids.end(), rhs.ids.begin(), rhs.ids.end(), gal::DerefEq{});
  }

  bool operator==(const NestedGenericIDList& lhs, const NestedGenericIDList& rhs) noexcept {
    auto lhs_ids = lhs.ids();
    auto rhs_ids = rhs.ids();

    return std::equal(lhs_ids.begin(), lhs_ids.end(), rhs_ids.begin(), rhs_ids.end());
  }

  void IdentifierExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void IdentifierExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool IdentifierExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const IdentifierExpression&>(other);

    return id() == result.id()
           && std::equal(generic_params_.begin(),
               generic_params_.end(),
               result.generic_params_.begin(),
               result.generic_params_.end(),
               gal::DerefEq{})
           && gal::unwrapping_equal(nested(), result.nested(), gal::DerefEq{});
  }

  std::unique_ptr<Expression> IdentifierExpression::internal_clone() const noexcept {
    return std::make_unique<IdentifierExpression>(loc(), id(), internal::clone_generics(generic_params_), nested_);
  }

  void LocalIdentifierExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void LocalIdentifierExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool LocalIdentifierExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const LocalIdentifierExpression&>(other);

    return name() == result.name();
  }

  std::unique_ptr<Expression> LocalIdentifierExpression::internal_clone() const noexcept {
    return std::make_unique<LocalIdentifierExpression>(loc(), name_);
  }

  void UnqualifiedIdentifierExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void UnqualifiedIdentifierExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool UnqualifiedIdentifierExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const UnqualifiedIdentifierExpression&>(other);

    return id() == result.id()
           && std::equal(generic_params_.begin(),
               generic_params_.end(),
               result.generic_params_.begin(),
               result.generic_params_.end(),
               gal::DerefEq{})
           && gal::unwrapping_equal(nested(), result.nested(), gal::DerefEq{});
  }

  std::unique_ptr<Expression> UnqualifiedIdentifierExpression::internal_clone() const noexcept {
    return std::make_unique<UnqualifiedIdentifierExpression>(loc(),
        id(),
        internal::clone_generics(generic_params_),
        nested_);
  }

  void StaticCallExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void StaticCallExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool StaticCallExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const StaticCallExpression&>(other);
    auto self_args = args();
    auto res_args = result.args();

    return &callee() == &callee()
           && std::equal(self_args.begin(), self_args.end(), res_args.begin(), res_args.end(), gal::DerefEq<>{})
           && std::equal(generic_params_.begin(),
               generic_params_.end(),
               result.generic_params_.begin(),
               result.generic_params_.end(),
               gal::DerefEq{});
  }

  std::unique_ptr<Expression> StaticCallExpression::internal_clone() const noexcept {
    return std::make_unique<StaticCallExpression>(loc(),
        callee(),
        id(),
        gal::clone_span(absl::MakeConstSpan(args_)),
        internal::clone_generics(generic_params_));
  }
  std::unique_ptr<StaticCallExpression> StaticCallExpression::from_call(const FullyQualifiedID& id,
      const Overload& overload,
      CallExpression* call) noexcept {
    // steal args instead of cloning just to toss the originals away later
    return std::make_unique<ast::StaticCallExpression>(call->loc(),
        overload,
        id,
        std::move(call->args_),
        std::move(call->generic_params_));
  }

  void CallExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void CallExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool CallExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const CallExpression&>(other);
    auto self_args = args();
    auto res_args = result.args();

    return callee() == result.callee()
           && std::equal(self_args.begin(), self_args.end(), res_args.begin(), res_args.end(), gal::DerefEq<>{})
           && std::equal(generic_params_.begin(),
               generic_params_.end(),
               result.generic_params_.begin(),
               result.generic_params_.end(),
               gal::DerefEq{});
  }

  std::unique_ptr<Expression> CallExpression::internal_clone() const noexcept {
    return std::make_unique<CallExpression>(loc(),
        callee().clone(),
        gal::clone_span(absl::MakeConstSpan(args_)),
        internal::clone_generics(generic_params_));
  }

  void MethodCallExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void MethodCallExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool MethodCallExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const MethodCallExpression&>(other);
    auto self_args = args();
    auto res_args = result.args();

    return object() == result.object() && method_name() == result.method_name()
           && std::equal(self_args.begin(), self_args.end(), res_args.begin(), res_args.end(), gal::DerefEq<>{})
           && std::equal(generic_params_.begin(),
               generic_params_.end(),
               result.generic_params_.begin(),
               result.generic_params_.end(),
               gal::DerefEq{});
  }

  std::unique_ptr<Expression> MethodCallExpression::internal_clone() const noexcept {
    return std::make_unique<MethodCallExpression>(loc(),
        object().clone(),
        std::string{method_name()},
        gal::clone_span(absl::MakeConstSpan(args_)),
        internal::clone_generics(generic_params_));
  }

  void IndexExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void IndexExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool IndexExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const IndexExpression&>(other);
    auto self_args = args();
    auto other_args = result.args();

    return callee() == result.callee()
           && std::equal(self_args.begin(),
               self_args.end(),
               other_args.begin(),
               other_args.end(),
               [](const std::unique_ptr<Expression>& lhs, const std::unique_ptr<Expression>& rhs) {
                 return *lhs == *rhs;
               });
  }

  std::unique_ptr<Expression> IndexExpression::internal_clone() const noexcept {
    return std::make_unique<IndexExpression>(loc(), callee().clone(), gal::clone_span(absl::MakeConstSpan(args_)));
  }

  void FieldAccessExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void FieldAccessExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool FieldAccessExpression::internal_equals(const Expression& expression) const noexcept {
    auto& result = internal::debug_cast<const FieldAccessExpression&>(expression);

    return object() == result.object() && field_name() == result.field_name();
  }

  std::unique_ptr<Expression> FieldAccessExpression::internal_clone() const noexcept {
    return std::make_unique<FieldAccessExpression>(loc(), object().clone(), std::string{field_name()});
  }

  void GroupExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void GroupExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool GroupExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const GroupExpression&>(other);

    return expr() == result.expr();
  }

  std::unique_ptr<Expression> GroupExpression::internal_clone() const noexcept {
    return std::make_unique<GroupExpression>(loc(), expr().clone());
  }

  void UnaryExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void UnaryExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool UnaryExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const UnaryExpression&>(other);

    return op() == result.op() && expr() == result.expr();
  }

  std::unique_ptr<Expression> UnaryExpression::internal_clone() const noexcept {
    return std::make_unique<UnaryExpression>(loc(), op(), expr().clone());
  }

  void BinaryExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BinaryExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BinaryExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const BinaryExpression&>(other);

    return op() == result.op() && lhs() == result.lhs() && rhs() == result.rhs();
  }

  std::unique_ptr<Expression> BinaryExpression::internal_clone() const noexcept {
    return std::make_unique<BinaryExpression>(loc(), op(), lhs().clone(), rhs().clone());
  }

  void CastExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void CastExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool CastExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const CastExpression&>(other);

    return unsafe() == result.unsafe() && castee() == result.castee() && cast_to() == result.cast_to();
  }

  std::unique_ptr<Expression> CastExpression::internal_clone() const noexcept {
    return std::make_unique<CastExpression>(loc(), unsafe(), castee().clone(), cast_to().clone());
  }

  void BlockExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BlockExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BlockExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const BlockExpression&>(other);
    auto stmts = statements();
    auto other_stmts = result.statements();

    return std::equal(stmts.begin(),
        stmts.end(),
        other_stmts.begin(),
        other_stmts.end(),
        [](const std::unique_ptr<ast::Statement>& lhs, const std::unique_ptr<ast::Statement>& rhs) {
          return *lhs == *rhs;
        });
  }

  std::unique_ptr<Expression> BlockExpression::internal_clone() const noexcept {
    return std::make_unique<BlockExpression>(loc(), gal::clone_span(statements()));
  }

  BlockExpression::BlockExpression(SourceLoc loc, std::vector<std::unique_ptr<Statement>> statements) noexcept
      : Expression(std::move(loc), ExprType::block),
        statements_{std::move(statements)} {}

  void IfThenExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void IfThenExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool IfThenExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const IfThenExpression&>(other);

    return condition() == result.condition()        //
           && true_branch() == result.true_branch() //
           && false_branch() == result.false_branch();
  }

  std::unique_ptr<Expression> IfThenExpression::internal_clone() const noexcept {
    return std::make_unique<IfThenExpression>(loc(),
        condition().clone(),
        true_branch().clone(),
        false_branch().clone());
  }

  void IfElseExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void IfElseExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool IfElseExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const IfElseExpression&>(other);

    auto elifs = elif_blocks();
    auto other_elifs = result.elif_blocks();

    return condition() == result.condition() && block() == result.block()
           && std::equal(elifs.begin(), elifs.end(), other_elifs.begin(), other_elifs.end())
           && gal::unwrapping_equal(else_block(), result.else_block());
  }

  std::unique_ptr<Expression> IfElseExpression::internal_clone() const noexcept {
    return std::make_unique<IfElseExpression>(loc(),
        condition().clone(),
        gal::static_unique_cast<BlockExpression>(block().clone()),
        elif_blocks_,
        gal::clone_if(else_block_));
  }

  void LoopExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void LoopExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool LoopExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const LoopExpression&>(other);

    return body() == result.body();
  }

  std::unique_ptr<Expression> LoopExpression::internal_clone() const noexcept {
    return std::make_unique<LoopExpression>(loc(), gal::static_unique_cast<BlockExpression>(body().clone()));
  }

  void WhileExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void WhileExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool WhileExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const WhileExpression&>(other);

    return condition() == result.condition() && body() == result.body();
  }

  std::unique_ptr<Expression> WhileExpression::internal_clone() const noexcept {
    return std::make_unique<WhileExpression>(loc(),
        condition().clone(),
        gal::static_unique_cast<BlockExpression>(body().clone()));
  }

  void ForExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ForExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ForExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const ForExpression&>(other);

    return loop_variable() == result.loop_variable() && loop_direction() == result.loop_direction()
           && init() == result.init() && last() == result.last() && body() == result.body();
  }

  std::unique_ptr<Expression> ForExpression::internal_clone() const noexcept {
    return std::make_unique<ForExpression>(loc(),
        loop_variable_,
        loop_direction(),
        init().clone(),
        last().clone(),
        gal::static_unique_cast<BlockExpression>(body().clone()));
  }

  void ReturnExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ReturnExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ReturnExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const ReturnExpression&>(other);

    return gal::unwrapping_equal(value(), result.value());
  }

  std::unique_ptr<Expression> ReturnExpression::internal_clone() const noexcept {
    auto val = value();
    auto new_value = val.has_value() ? std::make_optional((*val)->clone()) : std::nullopt;

    return std::make_unique<ReturnExpression>(loc(), std::move(new_value));
  }

  void BreakExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BreakExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BreakExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const BreakExpression&>(other);

    return gal::unwrapping_equal(value(), result.value());
  }

  std::unique_ptr<Expression> BreakExpression::internal_clone() const noexcept {
    auto val = value();
    auto new_value = val.has_value() ? std::make_optional((*val)->clone()) : std::nullopt;

    return std::make_unique<ReturnExpression>(loc(), std::move(new_value));
  }

  void ContinueExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ContinueExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ContinueExpression::internal_equals(const Expression& other) const noexcept {
    (void)internal::debug_cast<const ReturnExpression&>(other);

    return true;
  }

  std::unique_ptr<Expression> ContinueExpression::internal_clone() const noexcept {
    return std::make_unique<ContinueExpression>(loc());
  }

  bool operator==(const FieldInitializer& lhs, const FieldInitializer& rhs) noexcept {
    return lhs.name() == rhs.name() && lhs.init() == rhs.init();
  }

  void StructExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void StructExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool StructExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const StructExpression&>(other);

    return struct_type() == result.struct_type() && fields_ == result.fields_;
  }

  std::unique_ptr<Expression> StructExpression::internal_clone() const noexcept {
    return std::make_unique<StructExpression>(loc(), struct_type().clone(), fields_);
  }

  void ErrorExpression::internal_accept(ExpressionVisitorBase*) {
    assert(false);
  }

  void ErrorExpression::internal_accept(ConstExpressionVisitorBase*) const {
    assert(false);
  }

  bool ErrorExpression::internal_equals(const Expression&) const noexcept {
    return true;
  }

  std::unique_ptr<Expression> ErrorExpression::internal_clone() const noexcept {
    return std::make_unique<ErrorExpression>();
  }

  internal::CopyOverloadWithoutDefinition::CopyOverloadWithoutDefinition(const Overload& overload)
      : overload_{std::make_unique<Overload>(overload)} {}

  internal::CopyOverloadWithoutDefinition::CopyOverloadWithoutDefinition(const CopyOverloadWithoutDefinition& other)
      : overload_{std::make_unique<Overload>(other.overload())} {}

  internal::CopyOverloadWithoutDefinition::~CopyOverloadWithoutDefinition() = default;

  ElifBlock::ElifBlock(const ElifBlock& other) noexcept
      : condition_{other.condition_->clone()},
        block_{gal::static_unique_cast<BlockExpression>(other.block_->clone())} {}

  void ImplicitConversionExpression::internal_accept(ExpressionVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ImplicitConversionExpression::internal_accept(ConstExpressionVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ImplicitConversionExpression::internal_equals(const Expression& other) const noexcept {
    auto& result = internal::debug_cast<const ImplicitConversionExpression&>(other);

    return expr() == result.expr() && cast_to() == result.cast_to();
  }

  std::unique_ptr<Expression> ImplicitConversionExpression::internal_clone() const noexcept {
    return std::make_unique<ImplicitConversionExpression>(expr().clone(), cast_to().clone());
  }
} // namespace gal::ast