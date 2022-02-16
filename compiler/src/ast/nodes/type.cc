//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./type.h"
#include "./declaration.h"

namespace gal::ast {
  void ReferenceType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ReferenceType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ReferenceType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<ReferenceType>(other);

    return mut() == result.mut() && referenced() == result.referenced();
  }

  std::unique_ptr<Type> ReferenceType::internal_clone() const noexcept {
    return std::make_unique<ReferenceType>(loc(), mut(), referenced().clone());
  }

  void SliceType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void SliceType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool SliceType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<SliceType>(other);

    return sliced() == result.sliced();
  }

  std::unique_ptr<Type> SliceType::internal_clone() const noexcept {
    return std::make_unique<SliceType>(loc(), mut(), sliced().clone());
  }

  void PointerType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void PointerType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool PointerType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<PointerType>(other);

    return mut() == result.mut() && pointed() == result.pointed();
  }

  std::unique_ptr<Type> PointerType::internal_clone() const noexcept {
    return std::make_unique<PointerType>(loc(), mut(), pointed().clone());
  }

  void BuiltinIntegralType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BuiltinIntegralType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BuiltinIntegralType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<BuiltinIntegralType>(other);

    return width() == result.width() && has_sign() == result.has_sign();
  }

  std::unique_ptr<Type> BuiltinIntegralType::internal_clone() const noexcept {
    return std::make_unique<BuiltinIntegralType>(loc(), has_sign(), width());
  }

  void BuiltinFloatType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BuiltinFloatType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BuiltinFloatType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<BuiltinFloatType>(other);

    return width() == result.width();
  }

  std::unique_ptr<Type> BuiltinFloatType::internal_clone() const noexcept {
    return std::make_unique<BuiltinFloatType>(loc(), width());
  }

  void BuiltinBoolType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BuiltinBoolType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BuiltinBoolType::internal_equals(const Type& other) const noexcept {
    (void)gal::as<BuiltinBoolType>(other);

    return true;
  }

  std::unique_ptr<Type> BuiltinBoolType::internal_clone() const noexcept {
    return std::make_unique<BuiltinBoolType>(loc());
  }

  void BuiltinByteType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BuiltinByteType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BuiltinByteType::internal_equals(const Type& other) const noexcept {
    (void)gal::as<BuiltinByteType>(other);

    return true;
  }

  std::unique_ptr<Type> BuiltinByteType::internal_clone() const noexcept {
    return std::make_unique<BuiltinByteType>(loc());
  }

  void BuiltinCharType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void BuiltinCharType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool BuiltinCharType::internal_equals(const Type& other) const noexcept {
    (void)gal::as<BuiltinCharType>(other);

    return true;
  }

  std::unique_ptr<Type> BuiltinCharType::internal_clone() const noexcept {
    return std::make_unique<BuiltinCharType>(loc());
  }

  void UnqualifiedUserDefinedType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void UnqualifiedUserDefinedType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool UnqualifiedUserDefinedType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<UnqualifiedUserDefinedType>(other);

    return id() == result.id()
           && gal::unwrapping_equal(generic_params(), result.generic_params(), internal::GenericArgsCmp{});
  }

  std::unique_ptr<Type> UnqualifiedUserDefinedType::internal_clone() const noexcept {
    return std::make_unique<UnqualifiedUserDefinedType>(loc(), id(), std::vector<std::unique_ptr<Type>>{});
  }

  void UserDefinedType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void UserDefinedType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool UserDefinedType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<UserDefinedType>(other);

    return decl() == result.decl() && id() == result.id()
           && gal::unwrapping_equal(generic_params(), result.generic_params(), internal::GenericArgsCmp{});
  }

  std::unique_ptr<Type> UserDefinedType::internal_clone() const noexcept {
    return std::make_unique<UserDefinedType>(loc(), decl_, id(), internal::clone_generics(generic_params_));
  }

  void FnPointerType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void FnPointerType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool FnPointerType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<FnPointerType>(other);
    auto self_args = args();
    auto other_args = result.args();

    return return_type() == result.return_type()
           && std::equal(self_args.begin(), self_args.end(), other_args.begin(), other_args.end(), gal::DerefEq{});
  }

  std::unique_ptr<Type> FnPointerType::internal_clone() const noexcept {
    return std::make_unique<FnPointerType>(loc(), gal::clone_span(absl::MakeConstSpan(args_)), return_type().clone());
  }

  void DynInterfaceType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void DynInterfaceType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool DynInterfaceType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<DynInterfaceType>(other);

    return id() == result.id()
           && gal::unwrapping_equal(generic_params(), result.generic_params(), internal::GenericArgsCmp{});
  }

  std::unique_ptr<Type> DynInterfaceType::internal_clone() const noexcept {
    return std::make_unique<DynInterfaceType>(loc(), id(), internal::clone_generics(generic_params_));
  }

  void UnqualifiedDynInterfaceType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void UnqualifiedDynInterfaceType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool UnqualifiedDynInterfaceType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<UnqualifiedDynInterfaceType>(other);

    return id() == result.id()
           && gal::unwrapping_equal(generic_params(), result.generic_params(), internal::GenericArgsCmp{});
  }

  std::unique_ptr<Type> UnqualifiedDynInterfaceType::internal_clone() const noexcept {
    return std::make_unique<UnqualifiedDynInterfaceType>(loc(), id(), internal::clone_generics(generic_params_));
  }

  void VoidType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void VoidType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool VoidType::internal_equals(const Type& other) const noexcept {
    (void)gal::as<VoidType>(other);

    return true;
  }

  std::unique_ptr<Type> VoidType::internal_clone() const noexcept {
    return std::make_unique<VoidType>(loc());
  }

  void NilPointerType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void NilPointerType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool NilPointerType::internal_equals(const Type&) const noexcept {
    return true;
  }

  std::unique_ptr<Type> NilPointerType::internal_clone() const noexcept {
    return std::make_unique<NilPointerType>(loc());
  }

  void ErrorType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ErrorType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ErrorType::internal_equals(const Type&) const noexcept {
    return true;
  }

  std::unique_ptr<Type> ErrorType::internal_clone() const noexcept {
    return std::make_unique<ErrorType>();
  }

  void UnsizedIntegerType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void UnsizedIntegerType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool UnsizedIntegerType::internal_equals(const Type& other) const noexcept {
    (void)gal::as<UnsizedIntegerType>(other);

    return true;
  }

  std::unique_ptr<Type> UnsizedIntegerType::internal_clone() const noexcept {
    return std::make_unique<ast::UnsizedIntegerType>(loc(), value());
  }

  void ArrayType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ArrayType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ArrayType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<ArrayType>(other);

    return size() == result.size() && element_type() == result.element_type();
  }

  std::unique_ptr<Type> ArrayType::internal_clone() const noexcept {
    return std::make_unique<ArrayType>(loc(), size(), element_type().clone());
  }

  void IndirectionType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void IndirectionType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool IndirectionType::internal_equals(const Type& other) const noexcept {
    auto& result = gal::as<IndirectionType>(other);

    return produced() == result.produced();
  }

  std::unique_ptr<Type> IndirectionType::internal_clone() const noexcept {
    return std::make_unique<IndirectionType>(loc(), produced().clone(), mut());
  }

  bool Type::is_indirection_to(const Type& type) const noexcept {
    if (this->type() == TypeType::reference) {
      return gal::as<ReferenceType>(type).referenced() == type;
    } else if (this->type() == TypeType::pointer) {
      return gal::as<PointerType>(type).pointed() == type;
    } else if (this->type() == TypeType::indirection) {
      return gal::as<IndirectionType>(type).produced() == type;
    } else {
      return false;
    }
  }

  const ast::Type& Type::accessed_type() const noexcept {
    switch (type()) {
      case TypeType::pointer: {
        auto& p = gal::as<ast::PointerType>(*this);

        return p.pointed();
      }
      case TypeType::reference: {
        auto& r = gal::as<ast::ReferenceType>(*this);

        return r.referenced();
      }
      case TypeType::indirection: {
        auto& i = gal::as<ast::IndirectionType>(*this);

        return i.produced();
      }
      default: return *this;
    }
  }
} // namespace gal::ast