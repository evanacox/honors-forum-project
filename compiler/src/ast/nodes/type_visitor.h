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
  class BuiltinCharType;
  class UnqualifiedUserDefinedType;
  class UserDefinedType;
  class FnPointerType;
  class UnqualifiedDynInterfaceType;
  class DynInterfaceType;
  class VoidType;

  class TypeVisitorBase {
  public:
    virtual void visit(ReferenceType*) = 0;

    virtual void visit(SliceType*) = 0;

    virtual void visit(PointerType*) = 0;

    virtual void visit(BuiltinIntegralType*) = 0;

    virtual void visit(BuiltinFloatType*) = 0;

    virtual void visit(BuiltinByteType*) = 0;

    virtual void visit(BuiltinBoolType*) = 0;

    virtual void visit(BuiltinCharType*) = 0;

    virtual void visit(UnqualifiedUserDefinedType*) = 0;

    virtual void visit(UserDefinedType*) = 0;

    virtual void visit(FnPointerType*) = 0;

    virtual void visit(UnqualifiedDynInterfaceType*) = 0;

    virtual void visit(DynInterfaceType*) = 0;

    virtual void visit(VoidType*) = 0;

    virtual ~TypeVisitorBase() = default;
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

    virtual void visit(const BuiltinCharType&) = 0;

    virtual void visit(const UnqualifiedUserDefinedType&) = 0;

    virtual void visit(const UserDefinedType&) = 0;

    virtual void visit(const FnPointerType&) = 0;

    virtual void visit(const UnqualifiedDynInterfaceType&) = 0;

    virtual void visit(const DynInterfaceType&) = 0;

    virtual void visit(const VoidType&) = 0;

    virtual ~ConstTypeVisitorBase() = default;
  };

  /// Represents a type visitor that returns a value
  ///
  /// \tparam T Represents a
  template <typename T> using TypeVisitor = ValueVisitor<T, TypeVisitorBase>;

  template <typename T> using ConstTypeVisitor = ValueVisitor<T, ConstTypeVisitorBase>;
} // namespace gal::ast
