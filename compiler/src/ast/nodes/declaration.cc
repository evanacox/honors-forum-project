//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./declaration.h"

namespace gal::ast {
  void ImportDeclaration::internal_accept(gal::ast::DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ImportDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ImportDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<ImportDeclaration>(other);

    return exported() == result.exported() && mod() == result.mod() && alias() == result.alias();
  }

  std::unique_ptr<Declaration> ImportDeclaration::internal_clone() const noexcept {
    return std::make_unique<ImportDeclaration>(loc(), exported(), mod(), alias_);
  }

  void ImportFromDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ImportFromDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ImportFromDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<ImportFromDeclaration>(other);
    auto self_entities = imported_entities();
    auto other_entities = result.imported_entities();

    return exported() == result.exported()
           && std::equal(self_entities.begin(), self_entities.end(), other_entities.begin(), other_entities.end());
  }

  std::unique_ptr<Declaration> ImportFromDeclaration::internal_clone() const noexcept {
    return std::make_unique<ImportFromDeclaration>(loc(), exported(), entities_);
  }

  bool operator==(const FnPrototype& lhs, const FnPrototype& rhs) noexcept {
    auto lhs_args = lhs.args();
    auto rhs_args = rhs.args();

    return lhs.name() == rhs.name() && lhs.self() == rhs.self()
           && std::equal(lhs_args.begin(), lhs_args.end(), rhs_args.begin(), rhs_args.end())
           && lhs.return_type() == rhs.return_type();
  }

  void FnDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void FnDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool FnDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<FnDeclaration>(other);

    return exported() == result.exported() && external() == result.external() && proto() == result.proto()
           && body() == result.body();
  }

  std::unique_ptr<Declaration> FnDeclaration::internal_clone() const noexcept {
    return std::make_unique<FnDeclaration>(loc(),
        exported(),
        external(),
        proto_,
        gal::static_unique_cast<BlockExpression>(body().clone()));
  }

  void MethodDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void MethodDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool MethodDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<MethodDeclaration>(other);

    return exported() == result.exported() && proto() == result.proto() && body() == result.body();
  }

  std::unique_ptr<Declaration> MethodDeclaration::internal_clone() const noexcept {
    return std::make_unique<MethodDeclaration>(loc(),
        exported(),
        proto_,
        gal::static_unique_cast<BlockExpression>(body().clone()));
  }

  void StructDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void StructDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool StructDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<StructDeclaration>(other);
    auto self_fields = fields();
    auto other_fields = result.fields();

    return exported() == result.exported() && name() == result.name()
           && std::equal(self_fields.begin(), self_fields.end(), other_fields.begin(), other_fields.end());
  }

  std::unique_ptr<Declaration> StructDeclaration::internal_clone() const noexcept {
    return std::make_unique<StructDeclaration>(loc(), exported(), name_, fields_);
  }

  void ClassDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ClassDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ClassDeclaration::internal_equals(const Declaration&) const noexcept {
    assert(false);

    return false;
  }

  std::unique_ptr<Declaration> ClassDeclaration::internal_clone() const noexcept {
    assert(false);

    return std::make_unique<ErrorDeclaration>();
  }

  void TypeDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void TypeDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool TypeDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<TypeDeclaration>(other);

    return name() == result.name() && aliased() == result.aliased();
  }

  std::unique_ptr<Declaration> TypeDeclaration::internal_clone() const noexcept {
    return std::make_unique<TypeDeclaration>(loc(), exported(), name_, aliased().clone());
  }

  void ExternalFnDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ExternalFnDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ExternalFnDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<ast::ExternalFnDeclaration>(other);

    return proto() == result.proto();
  }

  std::unique_ptr<Declaration> ExternalFnDeclaration::internal_clone() const noexcept {
    return std::make_unique<ast::ExternalFnDeclaration>(loc(), exported(), proto());
  }

  void ExternalDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ExternalDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ExternalDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<ExternalDeclaration>(other);

    return externals_ == result.externals_;
  }

  std::unique_ptr<Declaration> ExternalDeclaration::internal_clone() const noexcept {
    return std::make_unique<ExternalDeclaration>(loc(), exported(), gal::clone_span(externals()));
  }

  void ConstantDeclaration::internal_accept(DeclarationVisitorBase* visitor) {
    visitor->visit(this);
  }

  void ConstantDeclaration::internal_accept(ConstDeclarationVisitorBase* visitor) const {
    visitor->visit(*this);
  }

  bool ConstantDeclaration::internal_equals(const Declaration& other) const noexcept {
    auto& result = gal::as<ConstantDeclaration>(other);

    return name() == result.name() && hint() == result.hint() && initializer() == result.initializer();
  }

  std::unique_ptr<Declaration> ConstantDeclaration::internal_clone() const noexcept {
    return std::make_unique<ConstantDeclaration>(loc(), exported(), name_, hint().clone(), initializer().clone());
  }
} // namespace gal::ast
