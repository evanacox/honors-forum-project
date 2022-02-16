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

#include "./value_visitor.h"

namespace gal::ast {
  class BindingStatement;
  class ExpressionStatement;
  class AssertStatement;

  class StatementVisitorBase {
  public:
    virtual void visit(BindingStatement*) = 0;

    virtual void visit(ExpressionStatement*) = 0;

    virtual void visit(AssertStatement*) = 0;

    virtual ~StatementVisitorBase() = default;
  };

  class ConstStatementVisitorBase {
  public:
    virtual void visit(const BindingStatement&) = 0;

    virtual void visit(const ExpressionStatement&) = 0;

    virtual void visit(const AssertStatement&) = 0;

    virtual ~ConstStatementVisitorBase() = default;
  };

  template <typename T> using StatementVisitor = ValueVisitor<T, StatementVisitorBase>;

  template <typename T> using ConstStatementVisitor = ValueVisitor<T, ConstStatementVisitorBase>;
} // namespace gal::ast
