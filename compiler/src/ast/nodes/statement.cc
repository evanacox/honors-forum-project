//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./statement.h"

namespace gal::ast {
  void BindingStatement::internal_accept(StatementVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BindingStatement::internal_accept(ConstStatementVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BindingStatement::internal_equals(const Statement& other) const noexcept {
    auto& result = gal::as<BindingStatement>(other);

    return name() == result.name() && gal::unwrapping_equal(hint(), result.hint(), gal::DerefEq{})
           && initializer() == result.initializer();
  }

  std::unique_ptr<Statement> BindingStatement::internal_clone() const noexcept {
    return std::make_unique<BindingStatement>(loc(), name_, mut(), initializer().clone(), gal::clone_if(hint_));
  }

  void AssertStatement::internal_accept(StatementVisitorBase* visitor) {
    visitor->visit(this);
  }

  void AssertStatement::internal_accept(ConstStatementVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool AssertStatement::internal_equals(const Statement& other) const noexcept {
    auto& result = gal::as<AssertStatement>(other);

    return assertion() == result.assertion() && message() == result.message();
  }

  std::unique_ptr<Statement> AssertStatement::internal_clone() const noexcept {
    return std::make_unique<AssertStatement>(loc(),
        assertion().clone(),
        gal::static_unique_cast<StringLiteralExpression>(message().clone()));
  }

  void ExpressionStatement::internal_accept(StatementVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ExpressionStatement::internal_accept(ConstStatementVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ExpressionStatement::internal_equals(const Statement& other) const noexcept {
    auto& result = gal::as<ExpressionStatement>(other);

    return expr() == result.expr();
  }

  std::unique_ptr<Statement> ExpressionStatement::internal_clone() const noexcept {
    return std::make_unique<ExpressionStatement>(loc(), expr().clone());
  }
} // namespace gal::ast
