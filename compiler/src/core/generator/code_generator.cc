//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./code_generator.h"
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/APInt.h"
#include "llvm/ADT/APSInt.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Scalar.h"
#include <algorithm>

namespace ast = gal::ast;
using TypeNamePair = std::pair<llvm::Type*, std::string_view>;

namespace {
  class IntoConstant final : public ast::ConstExpressionVisitor<llvm::Constant*> {
  public:
    explicit IntoConstant(gal::backend::CodeGenerator* gen, llvm::Type* type) noexcept : type_{type}, gen_{gen} {}

    void visit(const ast::StringLiteralExpression& expr) final {
      return_value(gen_->generate_string_literal(expr));
    }

    void visit(const ast::IntegerLiteralExpression& expr) final {
      return_value(llvm::ConstantInt::get(type_, expr.value()));
    }

    void visit(const ast::FloatLiteralExpression& expr) final {
      return_value(llvm::ConstantFP::get(type_, expr.value()));
    }

    void visit(const ast::BoolLiteralExpression& expr) final {
      return_value(llvm::ConstantInt::get(type_, expr.value() ? 1 : 0));
    }

    void visit(const ast::CharLiteralExpression& expr) final {
      return_value(llvm::ConstantInt::get(type_, expr.value()));
    }

    void visit(const ast::NilLiteralExpression&) final {
      return_value(llvm::Constant::getNullValue(type_));
    }

    void visit(const ast::ArrayExpression& expr) final {
      auto vec = std::vector<llvm::Constant*>{};

      for (auto& element : expr.elements()) {
        vec.push_back(element->accept(this));
      }

      return_value(llvm::ConstantArray::get(llvm::cast<llvm::ArrayType>(type_), vec));
    }

    void visit(const ast::UnqualifiedIdentifierExpression&) final {
      assert(false);
    }

    void visit(const ast::IdentifierExpression&) final {
      assert(false);
    }

    void visit(const ast::StaticGlobalExpression&) final {
      assert(false);
    }

    void visit(const ast::LocalIdentifierExpression&) final {
      assert(false);
    }

    void visit(const ast::StructExpression&) final {
      assert(false);
    }

    void visit(const ast::CallExpression&) final {
      assert(false);
    }

    void visit(const ast::StaticCallExpression&) final {
      assert(false);
    }

    void visit(const ast::MethodCallExpression&) final {
      assert(false);
    }

    void visit(const ast::StaticMethodCallExpression&) final {
      assert(false);
    }

    void visit(const ast::IndexExpression&) final {
      assert(false);
    }

    void visit(const ast::FieldAccessExpression&) final {
      assert(false);
    }

    void visit(const ast::GroupExpression&) final {
      assert(false);
    }

    void visit(const ast::UnaryExpression&) final {
      assert(false);
    }

    void visit(const ast::BinaryExpression&) final {
      assert(false);
    }

    void visit(const ast::CastExpression&) final {
      assert(false);
    }

    void visit(const ast::IfThenExpression&) final {
      assert(false);
    }

    void visit(const ast::IfElseExpression&) final {
      assert(false);
    }

    void visit(const ast::BlockExpression&) final {
      assert(false);
    }

    void visit(const ast::LoopExpression&) final {
      assert(false);
    }

    void visit(const ast::WhileExpression&) final {
      assert(false);
    }

    void visit(const ast::ForExpression&) final {
      assert(false);
    }

    void visit(const ast::ReturnExpression&) final {
      assert(false);
    }

    void visit(const ast::BreakExpression&) final {
      assert(false);
    }

    void visit(const ast::ContinueExpression&) final {
      assert(false);
    }

    void visit(const ast::ImplicitConversionExpression& expr) final {
      expr.expr().accept(
          this); // expr should only be literals, in which case it automagically gets turned to the right type anyway
    }

    void visit(const ast::LoadExpression&) final {
      assert(false);
    }

    void visit(const ast::AddressOfExpression&) final {
      assert(false);
    }

  private:
    llvm::Type* type_;
    gal::backend::CodeGenerator* gen_;
  };
} // namespace

namespace gal::backend {
  std::unique_ptr<llvm::Module> CodeGenerator::codegen() noexcept {
    return std::move(module_);
  }

  llvm::Function* CodeGenerator::codegen_proto(const ast::FnPrototype& proto,
      std::string_view name,
      bool external) noexcept {
    auto arg_types = std::vector<llvm::Type*>{};

    for (auto& arg : proto.args()) {
      arg_types.push_back(arg.type().accept(this));
    }

    auto* function_type = llvm::FunctionType::get(proto.return_type().accept(this), arg_types, false);
    auto* fn = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, llvm::Twine(name), module_.get());

    if (external) {
      fn->setCallingConv(llvm::CallingConv::C);
    } else {
      fn->setCallingConv(llvm::CallingConv::Fast);
    }

    return fn;
  }

  void CodeGenerator::visit(const ast::ImportDeclaration&) {}

  void CodeGenerator::visit(const ast::ImportFromDeclaration&) {}

  void CodeGenerator::visit(const ast::FnDeclaration& declaration) {
    auto* fn = codegen_proto(declaration.proto(), declaration.mangled_name(), declaration.external());
    auto* entry = llvm::BasicBlock::Create(*context_, "entry", fn);
    builder_->SetInsertPoint(entry);
  }

  void CodeGenerator::visit(const ast::StructDeclaration&) {}

  void CodeGenerator::visit(const ast::ClassDeclaration&) {}

  void CodeGenerator::visit(const ast::TypeDeclaration&) {}

  void CodeGenerator::visit(const ast::MethodDeclaration&) {}

  void CodeGenerator::visit(const ast::ExternalFnDeclaration& declaration) {
    codegen_proto(declaration.proto(), declaration.mangled_name(), true);
  }

  void CodeGenerator::visit(const ast::ExternalDeclaration& declaration) {
    for (auto& fn : declaration.externals()) {
      fn->accept(this);
    }
  }

  void CodeGenerator::visit(const ast::ConstantDeclaration& declaration) {
    auto* type = static_cast<llvm::Type*>(nullptr);
    auto* initializer = static_cast<llvm::Constant*>(nullptr);
    auto* constant = module_->getOrInsertGlobal(declaration.mangled_name(), type);

    llvm::cast<llvm::GlobalVariable>(constant)->setInitializer(initializer);
  }

  void CodeGenerator::visit(const ast::ReferenceType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::SliceType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::PointerType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::BuiltinIntegralType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::BuiltinFloatType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::BuiltinByteType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::BuiltinBoolType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::BuiltinCharType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::UnqualifiedUserDefinedType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::UserDefinedType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::FnPointerType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::UnqualifiedDynInterfaceType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::DynInterfaceType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::VoidType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::NilPointerType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::ErrorType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::UnsizedIntegerType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::ArrayType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::IndirectionType& type) {
    Type::return_value(map_type(type));
  }

  void CodeGenerator::visit(const ast::StringLiteralExpression& expression) {
    Expr::return_value(generate_string_literal(expression));
  }

  void CodeGenerator::visit(const ast::IntegerLiteralExpression& expression) {
    auto* type = expression.result().accept(this);

    Expr::return_value(into_constant(type, expression));
  }

  void CodeGenerator::visit(const ast::FloatLiteralExpression& expression) {
    auto* type = expression.result().accept(this);

    Expr::return_value(into_constant(type, expression));
  }

  void CodeGenerator::visit(const ast::BoolLiteralExpression& expression) {
    Expr::return_value(into_constant(llvm::Type::getInt1Ty(*context_), expression));
  }

  void CodeGenerator::visit(const ast::CharLiteralExpression& expression) {
    Expr::return_value(into_constant(llvm::Type::getInt8Ty(*context_), expression));
  }

  void CodeGenerator::visit(const ast::NilLiteralExpression& expression) {
    Expr::return_value(into_constant(llvm::Type::getInt8PtrTy(*context_), expression));
  }

  void CodeGenerator::visit(const ast::ArrayExpression& expression) {
    auto* type = expression.result().accept(this);
    auto* alloca = builder_->CreateAlloca(type);
    auto count = 0;

    for (const auto& expr : expression.elements()) {
      auto* val = expr->accept(this);

      auto* ptr = builder_->CreateGEP(alloca,
          {llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), 0),
              llvm::ConstantInt::get(llvm::Type::getInt64Ty(*context_), count++)});

      builder_->CreateStore(val, ptr);
    }

    Expr::return_value(alloca);
  }

  void CodeGenerator::visit(const ast::UnqualifiedIdentifierExpression&) {
    assert(false);
  }

  void CodeGenerator::visit(const ast::IdentifierExpression&) {
    assert(false);
  }

  void CodeGenerator::visit(const ast::StaticGlobalExpression& expression) {
    auto& decl = gal::as<ast::ConstantDeclaration>(expression.decl());
    auto* global = module_->getOrInsertGlobal(decl.mangled_name(), map_type(expression.result()));

    Expr::return_value(builder_->CreateLoad(global));
  }

  void CodeGenerator::visit(const ast::LocalIdentifierExpression& expression) {}

  void CodeGenerator::visit(const ast::StructExpression& expression) {}

  void CodeGenerator::visit(const ast::CallExpression& expression) {}

  void CodeGenerator::visit(const ast::StaticCallExpression& expression) {}

  void CodeGenerator::visit(const ast::MethodCallExpression& expression) {}

  void CodeGenerator::visit(const ast::StaticMethodCallExpression& expression) {}

  void CodeGenerator::visit(const ast::IndexExpression& expression) {}

  void CodeGenerator::visit(const ast::FieldAccessExpression& expression) {}

  void CodeGenerator::visit(const ast::GroupExpression& expression) {}

  void CodeGenerator::visit(const ast::UnaryExpression& expression) {}

  void CodeGenerator::visit(const ast::BinaryExpression& expression) {}

  void CodeGenerator::visit(const ast::CastExpression& expression) {}

  void CodeGenerator::visit(const ast::IfThenExpression& expression) {}

  void CodeGenerator::visit(const ast::IfElseExpression& expression) {}

  void CodeGenerator::visit(const ast::BlockExpression& expression) {}

  void CodeGenerator::visit(const ast::LoopExpression& expression) {}

  void CodeGenerator::visit(const ast::WhileExpression& expression) {}

  void CodeGenerator::visit(const ast::ForExpression& expression) {}

  void CodeGenerator::visit(const ast::ReturnExpression& expression) {}

  void CodeGenerator::visit(const ast::BreakExpression& expression) {}

  void CodeGenerator::visit(const ast::ContinueExpression& expression) {}

  void CodeGenerator::visit(const ast::ImplicitConversionExpression& expression) {}

  void CodeGenerator::visit(const ast::LoadExpression& expression) {}

  void CodeGenerator::visit(const ast::AddressOfExpression& expression) {}

  void CodeGenerator::visit(const ast::BindingStatement& statement) {}

  void CodeGenerator::visit(const ast::ExpressionStatement& statement) {}

  void CodeGenerator::visit(const ast::AssertStatement& statement) {}

  std::string CodeGenerator::label_name() noexcept {
    return absl::StrCat(".L", gal::to_digits(curr_label_++));
  }

  void CodeGenerator::reset_label() noexcept {
    curr_label_ = 0;
  }

  llvm::Type* CodeGenerator::native_type() noexcept {
    return integer_of_width(layout_.getPointerSizeInBits());
  }

  llvm::Type* CodeGenerator::integer_of_width(std::int64_t width) noexcept {
    switch (width) {
      case 8: return llvm::Type::getInt8Ty(*context_);
      case 16: return llvm::Type::getInt16Ty(*context_);
      case 32: return llvm::Type::getInt32Ty(*context_);
      case 64: return llvm::Type::getInt64Ty(*context_);
      case 128: return llvm::Type::getInt128Ty(*context_);
      default: assert(false); return nullptr;
    }
  }

  llvm::Type* CodeGenerator::pointer_to(llvm::Type* type) noexcept {
    return llvm::PointerType::get(type, layout_.getProgramAddressSpace());
  }

  llvm::Type* CodeGenerator::slice_of(llvm::Type* type) noexcept {
    return llvm::StructType::get(*context_, {native_type(), type});
  }

  llvm::Type* CodeGenerator::array_of(llvm::Type* type, std::uint64_t length) noexcept {
    return llvm::ArrayType::get(type, length);
  }

  llvm::Constant* CodeGenerator::generate_string_literal(const ast::StringLiteralExpression& literal) noexcept {
    if (auto it = string_literals_.find(literal.text_unquoted()); it != string_literals_.end()) {
      return it->second;
    }

    auto* type = array_of(integer_of_width(8), literal.text_unquoted().size());
    auto* constant = module_->getOrInsertGlobal(absl::StrCat(".str.", curr_str_++), type);
    auto* global = llvm::cast<llvm::GlobalVariable>(constant);
    global->setConstant(true);
    global->setLinkage(llvm::GlobalValue::InternalLinkage); // string literals are not visible outside the module
    global->setInitializer(llvm::ConstantDataArray::getString(*context_, literal.text_unquoted()));

    string_literals_.emplace(std::string{literal.text_unquoted()}, constant);

    return constant;
  }

  llvm::Type* CodeGenerator::map_type(const ast::Type& type) noexcept {
    switch (type.type()) {
      case ast::TypeType::builtin_integral: {
        auto& integral = gal::as<ast::BuiltinIntegralType>(type);

        if (auto width = ast::width_of(integral.width())) {
          return integer_of_width(*width);
        } else {
          return native_type();
        }
      }
      case ast::TypeType::builtin_float: {
        auto& floating_pt = gal::as<ast::BuiltinFloatType>(type);

        switch (floating_pt.width()) {
          case ast::FloatWidth::ieee_single: return llvm::Type::getFloatTy(*context_);
          case ast::FloatWidth::ieee_double: return llvm::Type::getDoubleTy(*context_);
          case ast::FloatWidth::ieee_quadruple: return llvm::Type::getFP128Ty(*context_);
          default: assert(false); return nullptr;
        }
      }
      case ast::TypeType::builtin_byte:
      case ast::TypeType::builtin_char: return llvm::Type::getInt8Ty(*context_);
      case ast::TypeType::builtin_bool: return llvm::Type::getInt1Ty(*context_);
      case ast::TypeType::array: {
        auto& array = gal::as<ast::ArrayType>(type);

        return array_of(map_type(array.element_type()), array.size());
      }
      case ast::TypeType::reference: {
        auto& ref = gal::as<ast::ReferenceType>(type);

        return pointer_to(map_type(ref.referenced()));
      }
      case ast::TypeType::slice: {
        auto& slice = gal::as<ast::SliceType>(type);

        return llvm::StructType::get(*context_, {pointer_to(map_type(slice.sliced())), native_type()});
      }
      case ast::TypeType::pointer: {
        auto& ptr = gal::as<ast::PointerType>(type);

        return pointer_to(map_type(ptr.pointed()));
      }
      case ast::TypeType::builtin_void: return llvm::Type::getVoidTy(*context_);
      case ast::TypeType::user_defined: return struct_for(gal::as<ast::UserDefinedType>(type));
      case ast::TypeType::fn_pointer: {
        auto& fn_ptr = gal::as<ast::FnPointerType>(type);
        auto vec = llvm::SmallVector<llvm::Type*, 8>{};

        for (auto& arg : fn_ptr.args()) {
          vec.push_back(map_type(*arg));
        }

        return llvm::FunctionType::get(map_type(fn_ptr.return_type()), vec, false);
      }
      case ast::TypeType::dyn_interface:
      case ast::TypeType::error:
      case ast::TypeType::nil_pointer:
      case ast::TypeType::unsized_integer:
      case ast::TypeType::indirection:
      case ast::TypeType::dyn_interface_unqualified:
      case ast::TypeType::user_defined_unqualified:
      default: assert(false); break;
    }
  }

  llvm::Type* CodeGenerator::struct_for(const ast::UserDefinedType& type) noexcept {
    auto entity = type.id().as_string();

    // either we've already generated an LLVM struct & lookup data, or we need to
    if (auto it = user_types_.find(entity); it != user_types_.end()) {
      return it->second;
    }

    auto fields = from_structure(gal::as<ast::StructDeclaration>(type.decl()));
    auto* llvm_type = struct_from(fields);

    user_types_.emplace(std::string{entity}, llvm_type);
    create_user_type_mapping(entity, fields);
  }

  std::size_t CodeGenerator::field_index(const ast::UserDefinedType& type, std::string_view name) noexcept {
    (void)struct_for(type);

    // default construct and then insert in
    auto& result = user_type_mapping_[type.id().as_string()];

    // the index of `name` is the index in the LLVM type of that field
    return std::distance(result.begin(), std::find(result.begin(), result.end(), name));
  }

  llvm::SmallVector<std::pair<llvm::Type*, std::string_view>, 8> CodeGenerator::from_structure(
      const ast::StructDeclaration& decl) noexcept {
    auto fields = llvm::SmallVector<TypeNamePair, 8>{};

    for (auto& field : decl.fields()) {
      auto* field_type = field.type().accept(this);

      fields.emplace_back(field_type, field.name());
    }

    std::stable_sort(fields.begin(), fields.end(), [this](const TypeNamePair& lhs, const TypeNamePair& rhs) {
      return layout_.getTypeAllocSize(lhs.first).getFixedSize() < layout_.getTypeAllocSize(rhs.first).getFixedSize();
    });
  }

  void CodeGenerator::create_user_type_mapping(std::string_view entity,
      llvm::ArrayRef<std::pair<llvm::Type*, std::string_view>> array) noexcept {
    // default construct a vec, then insert into it
    auto& vec = user_type_mapping_[entity];
    vec.reserve(array.size());
    std::transform(array.begin(), array.end(), std::back_inserter(vec), [](const TypeNamePair& pair) {
      return pair.second;
    });
  }

  llvm::Type* CodeGenerator::struct_from(llvm::ArrayRef<TypeNamePair> array) noexcept {
    auto field_types = llvm::SmallVector<llvm::Type*, 8>{};
    field_types.reserve(array.size());
    std::transform(array.begin(), array.end(), std::back_inserter(field_types), [](const TypeNamePair& pair) {
      return pair.first;
    });

    return llvm::StructType::get(*context_, field_types);
  }

  llvm::Constant* CodeGenerator::into_constant(llvm::Type* type, const ast::Expression& expr) noexcept {
    auto constant_generator = IntoConstant{this, type};

    return expr.accept(&constant_generator);
  }
} // namespace gal::backend