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
  class ImportDeclaration;
  class ImportFromDeclaration;
  class FnDeclaration;
  class StructDeclaration;
  class ClassDeclaration;
  class TypeDeclaration;
  class MethodDeclaration;
  class ExternalFnDeclaration;
  class ExternalDeclaration;
  class ConstantDeclaration;

  class DeclarationVisitorBase {
  public:
    virtual void visit(ImportDeclaration*) = 0;

    virtual void visit(ImportFromDeclaration*) = 0;

    virtual void visit(FnDeclaration*) = 0;

    virtual void visit(StructDeclaration*) = 0;

    virtual void visit(ClassDeclaration*) = 0;

    virtual void visit(TypeDeclaration*) = 0;

    virtual void visit(MethodDeclaration*) = 0;

    virtual void visit(ExternalFnDeclaration*) = 0;

    virtual void visit(ExternalDeclaration*) = 0;

    virtual void visit(ConstantDeclaration*) = 0;

    virtual ~DeclarationVisitorBase() = default;
  };

  class ConstDeclarationVisitorBase {
  public:
    virtual void visit(const ImportDeclaration&) = 0;

    virtual void visit(const ImportFromDeclaration&) = 0;

    virtual void visit(const FnDeclaration&) = 0;

    virtual void visit(const StructDeclaration&) = 0;

    virtual void visit(const ClassDeclaration&) = 0;

    virtual void visit(const TypeDeclaration&) = 0;

    virtual void visit(const MethodDeclaration&) = 0;

    virtual void visit(const ExternalFnDeclaration&) = 0;

    virtual void visit(const ExternalDeclaration&) = 0;

    virtual void visit(const ConstantDeclaration&) = 0;

    virtual ~ConstDeclarationVisitorBase() = default;
  };

  template <typename T> using DeclarationVisitor = ValueVisitor<T, DeclarationVisitorBase>;

  template <typename T> using ConstDeclarationVisitor = ValueVisitor<T, ConstDeclarationVisitorBase>;
} // namespace gal::ast