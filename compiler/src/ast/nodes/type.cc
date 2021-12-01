//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./type.h"

namespace gal::ast {
  void ReferenceType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ReferenceType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ReferenceType::internal_equals(const Type& other) const noexcept {
    auto& result = internal::debug_cast<const ReferenceType&>(other);

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
    auto& result = internal::debug_cast<const SliceType&>(other);

    return sliced() == result.sliced();
  }

  std::unique_ptr<Type> SliceType::internal_clone() const noexcept {
    return std::make_unique<SliceType>(loc(), sliced().clone());
  }

  void PointerType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void PointerType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool PointerType::internal_equals(const Type& other) const noexcept {
    auto& result = internal::debug_cast<const PointerType&>(other);

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
    auto& result = internal::debug_cast<const BuiltinIntegralType&>(other);

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
    auto& result = internal::debug_cast<const BuiltinFloatType&>(other);

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
    (void)internal::debug_cast<const BuiltinBoolType&>(other);

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
    (void)internal::debug_cast<const BuiltinByteType&>(other);

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
    (void)internal::debug_cast<const BuiltinCharType&>(other);

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
    auto& result = internal::debug_cast<const UnqualifiedUserDefinedType&>(other);

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
    auto& result = internal::debug_cast<const UserDefinedType&>(other);

    return id() == result.id()
           && gal::unwrapping_equal(generic_params(), result.generic_params(), internal::GenericArgsCmp{});
  }

  std::unique_ptr<Type> UserDefinedType::internal_clone() const noexcept {
    return std::make_unique<UserDefinedType>(loc(), id(), internal::clone_generics(generic_params_));
  }

  void FnPointerType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void FnPointerType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool FnPointerType::internal_equals(const Type& other) const noexcept {
    auto& result = internal::debug_cast<const FnPointerType&>(other);
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
    auto& result = internal::debug_cast<const DynInterfaceType&>(other);

    return id() == result.id()
           && gal::unwrapping_equal(generic_params(), result.generic_params(), internal::GenericArgsCmp{});
  }

  std::unique_ptr<Type> DynInterfaceType::internal_clone() const noexcept {
    return std::make_unique<UserDefinedType>(loc(), id(), internal::clone_generics(generic_params_));
  }

  void UnqualifiedDynInterfaceType::internal_accept(TypeVisitorBase* visitor) {
    visitor->visit(this);
  }

  void UnqualifiedDynInterfaceType::internal_accept(ConstTypeVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool UnqualifiedDynInterfaceType::internal_equals(const Type& other) const noexcept {
    auto& result = internal::debug_cast<const UnqualifiedDynInterfaceType&>(other);

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
    (void)internal::debug_cast<const VoidType&>(other);

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
    (void)internal::debug_cast<const UnsizedIntegerType&>(other);

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
    auto& result = internal::debug_cast<const ArrayType&>(other);

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
    auto& result = internal::debug_cast<const IndirectionType&>(other);

    return produced() == result.produced();
  }

  std::unique_ptr<Type> IndirectionType::internal_clone() const noexcept {
    return std::unique_ptr<Type>();
  }
} // namespace gal::ast