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

#include "./nodes/declaration_visitor.h"
#include "./nodes/expression_visitor.h"
#include "./nodes/statement_visitor.h"
#include "./nodes/type_visitor.h"

namespace gal::ast {
  template <typename T>
  class AnyVisitor : public DeclarationVisitor<T>,
                     public ExpressionVisitor<T>,
                     public StatementVisitor<T>,
                     public TypeVisitor<T> {};
} // namespace gal::ast