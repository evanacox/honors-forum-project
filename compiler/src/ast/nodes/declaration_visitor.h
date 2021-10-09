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

namespace gal::ast {
  class ImportDeclaration;
  class ImportFromDeclaration;
  class FnDeclaration;
  class StructDeclaration;
  class ClassDeclaration;
  class TypeDeclaration;

  class DeclarationVisitor {
  public:
    virtual void visit(ImportDeclaration*) = 0;

    virtual void visit(ImportFromDeclaration*) = 0;

    virtual void visit(FnDeclaration*) = 0;

    virtual void visit(StructDeclaration*) = 0;

    virtual void visit(ClassDeclaration*) = 0;

    virtual void visit(TypeDeclaration*) = 0;
  };
} // namespace gal::ast