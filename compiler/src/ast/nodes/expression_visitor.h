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

#include "./value_visitor.h"

namespace gal::ast {
  class StringLiteralExpression;
  class IntegerLiteralExpression;
  class FloatLiteralExpression;
  class BoolLiteralExpression;
  class CharLiteralExpression;
  class NilLiteral;
  class IdentifierExpression;
  class CallExpression;
  class IndexExpression;
  class FieldAccessExpression;
  class GroupExpression;
  class UnaryExpression;
  class BinaryExpression;
  class CastExpression;
  class IfThenExpression;
  class IfElseExpression;
  class BlockExpression;
  class LoopExpression;
  class WhileExpression;
  class ForExpression;
  class ReturnExpression;

  class ExpressionVisitorBase {
  public:
    virtual void visit(StringLiteralExpression*) = 0;

    virtual void visit(IntegerLiteralExpression*) = 0;

    virtual void visit(FloatLiteralExpression*) = 0;

    virtual void visit(BoolLiteralExpression*) = 0;

    virtual void visit(CharLiteralExpression*) = 0;

    virtual void visit(NilLiteral*) = 0;

    virtual void visit(IdentifierExpression*) = 0;

    virtual void visit(CallExpression*) = 0;

    virtual void visit(IndexExpression*) = 0;

    virtual void visit(FieldAccessExpression*) = 0;

    virtual void visit(GroupExpression*) = 0;

    virtual void visit(UnaryExpression*) = 0;

    virtual void visit(BinaryExpression*) = 0;

    virtual void visit(CastExpression*) = 0;

    virtual void visit(IfThenExpression*) = 0;

    virtual void visit(IfElseExpression*) = 0;

    virtual void visit(BlockExpression*) = 0;

    virtual void visit(LoopExpression*) = 0;

    virtual void visit(WhileExpression*) = 0;

    virtual void visit(ForExpression*) = 0;

    virtual void visit(ReturnExpression*) = 0;

    virtual ~ExpressionVisitorBase() = default;
  };

  class ConstExpressionVisitorBase {
  public:
    virtual void visit(const StringLiteralExpression&) = 0;

    virtual void visit(const IntegerLiteralExpression&) = 0;

    virtual void visit(const FloatLiteralExpression&) = 0;

    virtual void visit(const BoolLiteralExpression&) = 0;

    virtual void visit(const CharLiteralExpression&) = 0;

    virtual void visit(const NilLiteral&) = 0;

    virtual void visit(const IdentifierExpression&) = 0;

    virtual void visit(const CallExpression&) = 0;

    virtual void visit(const IndexExpression&) = 0;

    virtual void visit(const FieldAccessExpression&) = 0;

    virtual void visit(const GroupExpression&) = 0;

    virtual void visit(const UnaryExpression&) = 0;

    virtual void visit(const BinaryExpression&) = 0;

    virtual void visit(const CastExpression&) = 0;

    virtual void visit(const IfThenExpression&) = 0;

    virtual void visit(const IfElseExpression&) = 0;

    virtual void visit(const BlockExpression&) = 0;

    virtual void visit(const LoopExpression&) = 0;

    virtual void visit(const WhileExpression&) = 0;

    virtual void visit(const ForExpression&) = 0;

    virtual void visit(const ReturnExpression&) = 0;

    virtual ~ConstExpressionVisitorBase() = default;
  };

  template <typename T> class ExpressionVisitor : public ValueVisitor<T, ExpressionVisitorBase> {};

  template <typename T> class ConstExpressionVisitor : public ValueVisitor<T, ConstExpressionVisitorBase> {};
} // namespace gal::ast
