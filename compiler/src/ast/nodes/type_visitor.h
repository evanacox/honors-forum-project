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
  class ReferenceType;
  class SliceType;
  class PointerType;
  class BuiltinIntegralType;
  class BuiltinFloatType;
  class BuiltinByteType;
  class BuiltinBoolType;
  class UserDefinedType;
  class FnPointerType;
  class DynInterfaceType;

  class TypeVisitorBase {
  public:
    virtual void visit(ReferenceType*) = 0;

    virtual void visit(SliceType*) = 0;

    virtual void visit(PointerType*) = 0;

    virtual void visit(BuiltinIntegralType*) = 0;

    virtual void visit(BuiltinFloatType*) = 0;

    virtual void visit(BuiltinByteType*) = 0;

    virtual void visit(BuiltinBoolType*) = 0;

    virtual void visit(UserDefinedType*) = 0;

    virtual void visit(FnPointerType*) = 0;

    virtual void visit(DynInterfaceType*) = 0;
  };

  class ConstTypeVisitorBase {
  public:
    virtual void visit(const ReferenceType&) = 0;

    virtual void visit(const SliceType&) = 0;

    virtual void visit(const PointerType&) = 0;

    virtual void visit(const BuiltinIntegralType&) = 0;

    virtual void visit(const BuiltinFloatType&) = 0;

    virtual void visit(const BuiltinByteType&) = 0;

    virtual void visit(const BuiltinBoolType&) = 0;

    virtual void visit(const UserDefinedType&) = 0;

    virtual void visit(const FnPointerType&) = 0;

    virtual void visit(const DynInterfaceType&) = 0;
  };

  template <typename T> class TypeVisitor : public ValueVisitor<T, TypeVisitorBase> {};

  template <typename T> class ConstTypeVisitor : public ValueVisitor<T, ConstTypeVisitorBase> {};
} // namespace gal::ast
