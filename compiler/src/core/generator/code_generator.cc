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
#include "../name_resolver.h"
#include "../typechecker.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
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
  CodeGenerator::CodeGenerator(llvm::LLVMContext* context,
      const ast::Program& program,
      const llvm::TargetMachine& machine) noexcept
      : context_{context},
        program_{program},
        machine_{machine},
        layout_{machine_.createDataLayout()},
        module_{std::make_unique<llvm::Module>("main", *context_)},
        builder_{std::make_unique<llvm::IRBuilder<>>(*context_)},
        variables_{builder_.get(), layout_} {}

  std::unique_ptr<llvm::Module> CodeGenerator::codegen() noexcept {
    module_->setTargetTriple(machine_.getTargetTriple().getTriple());
    module_->setDataLayout(layout_);

    // everything besides functions can be defined right now,
    // but functions are just declared, so we can call them later
    for (auto& decl : program_.decls()) {
      if (decl->is(ast::DeclType::fn_decl)) {
        auto& fn = gal::as<ast::FnDeclaration>(*decl);

        codegen_proto(fn.proto(), fn.mangled_name());
      } else {
        decl->accept(this);
      }
    }

    // we now go back and actually codegen for each function now that it's safe to generate calls
    for (auto& decl : program_.decls()) {
      if (decl->is(ast::DeclType::fn_decl)) {
        // cast isn't strictly necessary, but it allows devirtualizing the `accept` call. can't hurt!
        auto& fn = gal::as<ast::FnDeclaration>(*decl);

        fn.accept(this);

#ifndef NDEBUG
        assert(!llvm::verifyFunction(*module_->getFunction(fn.mangled_name()), &llvm::outs()));
#endif
      }
    }

    return std::move(module_);
  }

  llvm::Function* CodeGenerator::codegen_proto(const ast::FnPrototype& proto, std::string_view name) noexcept {
    if (auto* ptr = module_->getFunction(name); ptr != nullptr) {
      return ptr;
    }

    auto arg_types = std::vector<llvm::Type*>{};

    for (auto& arg : proto.args()) {
      arg_types.push_back(arg.type().accept(this));
    }

    auto* function_type = llvm::FunctionType::get(proto.return_type().accept(this), arg_types, false);
    auto* fn = llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, name, module_.get());

    return fn;
  }

  void CodeGenerator::visit(const ast::ImportDeclaration&) {}

  void CodeGenerator::visit(const ast::ImportFromDeclaration&) {}

  void CodeGenerator::visit(const ast::FnDeclaration& declaration) {
    auto is_void = declaration.proto().return_type().is(ast::TypeType::builtin_void);
    auto* fn = codegen_proto(declaration.proto(), declaration.mangled_name());
    auto* entry = llvm::BasicBlock::Create(*context_, "entry", fn);
    exit_block_ = llvm::BasicBlock::Create(*context_, "exit", fn);
    builder_->SetInsertPoint(entry);

    if (!is_void) {
      auto* type = map_type(declaration.proto().return_type());
      return_value_ = builder_->CreateAlloca(type);
      builder_->SetInsertPoint(exit_block_);
      builder_->CreateRet(builder_->CreateLoad(type, return_value_));
    } else {
      builder_->SetInsertPoint(exit_block_);
      builder_->CreateRetVoid();
    }

    builder_->SetInsertPoint(entry);

    variables_.enter_scope();
    auto fn_args = declaration.proto().args();
    auto it = fn_args.begin();

    // copy all args onto stack, so we don't have to special-case when trying to extract from params or whatever
    for (auto& arg : fn->args()) {
      auto* alloca = builder_->CreateAlloca(arg.getType());
      variables_.set((it++)->name(), alloca);
      builder_->CreateStore(&arg, alloca);
    }

    auto* last_expr = declaration.body().accept(this);

    if (!is_void && last_expr != nullptr) { // returns and similar will give `nullptr`, ignore them
      builder_->CreateStore(last_expr, return_value_);
    }

    variables_.leave_scope();
    builder_->CreateBr(exit_block_);
  }

  void CodeGenerator::visit(const ast::StructDeclaration&) {}

  void CodeGenerator::visit(const ast::ClassDeclaration&) {}

  void CodeGenerator::visit(const ast::TypeDeclaration&) {}

  void CodeGenerator::visit(const ast::MethodDeclaration&) {}

  void CodeGenerator::visit(const ast::ExternalFnDeclaration& declaration) {
    codegen_proto(declaration.proto(), declaration.mangled_name());
  }

  void CodeGenerator::visit(const ast::ExternalDeclaration& declaration) {
    for (auto& fn : declaration.externals()) {
      fn->accept(this);
    }
  }

  void CodeGenerator::visit(const ast::ConstantDeclaration& declaration) {
    auto* type = map_type(declaration.hint());
    auto* initializer = into_constant(type, declaration.initializer());
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
    auto* element_type = llvm::cast<llvm::ArrayType>(type)->getElementType();
    auto* alloca = builder_->CreateAlloca(type);
    auto count = 0;

    for (const auto& expr : expression.elements()) {
      auto* val = expr->accept(this);

      auto* ptr = builder_->CreateInBoundsGEP(element_type,
          alloca,
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
    auto* type = map_type(decl.hint());
    auto* global = module_->getGlobalVariable(decl.mangled_name());

    Expr::return_value(builder_->CreateLoad(type, global));
  }

  void CodeGenerator::visit(const ast::LocalIdentifierExpression& expression) {
    Expr::return_value(variables_.get(expression.name()));
  }

  void CodeGenerator::visit(const ast::StructExpression& expression) {
    auto* type = map_type(expression.result());
    auto* alloca = builder_->CreateAlloca(type);

    for (auto& field : expression.fields()) {
      auto* initializer = codegen_into_reg(field.init());
      auto* gep = builder_->CreateInBoundsGEP(type,
          alloca,
          {int64_constant(0),
              int32_constant(static_cast<std::int32_t>(
                  field_index(gal::as<ast::UserDefinedType>(expression.result()), field.name())))});

      builder_->CreateStore(initializer, gep);
    }

    Expr::return_value(alloca);
  }

  void CodeGenerator::visit(const ast::CallExpression& expression) {
    auto* fn_type = map_type(expression.result());
    auto* callee = codegen_into_reg(expression.callee(), fn_type);
    auto args = llvm::SmallVector<llvm::Value*, 8>{};

    for (auto& arg : expression.args()) {
      args.push_back(codegen_into_reg(*arg));
    }

    Expr::return_value(builder_->CreateCall(llvm::cast<llvm::FunctionType>(fn_type), callee, args));
  }

  void CodeGenerator::visit(const ast::StaticCallExpression& expression) {
    auto args_types = llvm::SmallVector<llvm::Type*, 8>{};
    auto args = llvm::SmallVector<llvm::Value*, 8>{};

    for (auto& expr : expression.args()) {
      auto* type = map_type(expr->result());
      args_types.push_back(type);
      args.push_back(codegen_into_reg(*expr, type));
    }

    auto name = std::visit(
        [](auto* decl) {
          return decl->mangled_name();
        },
        expression.callee().decl());

    auto* fn = module_->getFunction(name);
    Expr::return_value(builder_->CreateCall(fn, args));
  }

  void CodeGenerator::visit(const ast::MethodCallExpression&) {}

  void CodeGenerator::visit(const ast::StaticMethodCallExpression&) {}

  void CodeGenerator::visit(const ast::IndexExpression& expression) {
    auto* array_type = map_type(expression.callee().result());
    auto* array = codegen_into_reg(expression.callee());
    auto* offset = expression.indices()[0]->accept(this); // TODO: multiple indices?
    auto* array_ptr = static_cast<llvm::Value*>(nullptr);

    if (expression.callee().result().is(ast::TypeType::slice)) {
      // extract ptr field
      array_ptr = builder_->CreateExtractValue(array, {0});
    } else {
      array_ptr = array;
    }

    Expr::return_value(builder_->CreateInBoundsGEP(array_type, array_ptr, {int64_constant(0), offset}));
  }

  void CodeGenerator::visit(const ast::FieldAccessExpression&) {}

  void CodeGenerator::visit(const ast::GroupExpression& expression) {
    Expr::return_value(expression.expr().accept(this));
  }

  void CodeGenerator::visit(const ast::UnaryExpression&) {}

  void CodeGenerator::visit(const ast::BinaryExpression&) {}

  void CodeGenerator::visit(const ast::CastExpression&) {}

  void CodeGenerator::visit(const ast::IfThenExpression&) {}

  void CodeGenerator::visit(const ast::IfElseExpression&) {}

  void CodeGenerator::visit(const ast::BlockExpression& expression) {
    variables_.enter_scope();

    auto* last_stmt_value = static_cast<llvm::Value*>(nullptr);
    for (auto& stmt : expression.statements()) {
      last_stmt_value = stmt->accept(this);
    }

    variables_.leave_scope();

    // while this will be `nullptr` for non-expr statements, the type checker will ensure
    // that if we actually need to **use** this "value" it will exist
    Expr::return_value(last_stmt_value);
  }

  void CodeGenerator::visit(const ast::LoopExpression&) {}

  void CodeGenerator::visit(const ast::WhileExpression&) {}

  void CodeGenerator::visit(const ast::ForExpression&) {}

  void CodeGenerator::visit(const ast::ReturnExpression& expression) {
    if (auto ptr = expression.value()) {
      auto* value = codegen_into_reg(**ptr);

      builder_->CreateStore(value, return_value_);
    }

    builder_->CreateBr(exit_block_);

    Expr::return_value(nullptr); // shouldn't be possible to actually *use* this
  }

  void CodeGenerator::visit(const ast::BreakExpression&) {}

  void CodeGenerator::visit(const ast::ContinueExpression&) {}

  void CodeGenerator::visit(const ast::ImplicitConversionExpression&) {}

  void CodeGenerator::visit(const ast::LoadExpression&) {}

  void CodeGenerator::visit(const ast::AddressOfExpression&) {}

  void CodeGenerator::visit(const ast::BindingStatement&) {}

  void CodeGenerator::visit(const ast::ExpressionStatement& statement) {
    Stmt::return_value(codegen_into_reg(statement.expr()));
  }

  void CodeGenerator::visit(const ast::AssertStatement&) {}

  std::string CodeGenerator::label_name() noexcept {
    return absl::StrCat("L", gal::to_digits(curr_label_++));
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
    auto* slice = llvm::StructType::get(*context_, {native_type(), type});
    slice->setName("slice");
    return slice;
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
    llvm::cast<llvm::StructType>(llvm_type)->setName(
        absl::StrCat("struct", absl::StrReplaceAll(entity, {{"::", "."}})));

    user_types_.emplace(std::string{entity}, llvm_type);
    create_user_type_mapping(entity, fields);

    return llvm_type;
  }

  std::uint32_t CodeGenerator::field_index(const ast::UserDefinedType& type, std::string_view name) noexcept {
    (void)struct_for(type);

    // default construct and then insert in
    auto& result = user_type_mapping_[type.id().as_string()];

    // the index of `name` is the index in the LLVM type of that field
    return static_cast<std::int32_t>(std::distance(result.begin(), std::find(result.begin(), result.end(), name)));
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

    return fields;
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

  llvm::Constant* CodeGenerator::int64_constant(std::int64_t value) noexcept {
    return llvm::ConstantInt::get(integer_of_width(64), value);
  }

  llvm::Constant* CodeGenerator::int32_constant(std::int32_t value) noexcept {
    return llvm::ConstantInt::get(integer_of_width(32), value);
  }

  llvm::Value* CodeGenerator::codegen_into_reg(const ast::Expression& expr, llvm::Type* type) noexcept {
    auto* inst = expr.accept(this);

    // if it's not a register value, create a load to it and return that
    if (inst->getType()->isPointerTy()) {
      return builder_->CreateLoad(type, inst);
    }

    return inst;
  }

  llvm::Value* CodeGenerator::codegen_into_reg(const ast::Expression& expr) noexcept {
    auto* inst = expr.accept(this);

    // if it's not a register value, create a load to it and return that
    if (inst != nullptr && inst->getType()->isPointerTy()) {
      // lazily generate the LLVM type if possible
      return builder_->CreateLoad(map_type(expr.result()), inst);
    }

    return inst;
  }
} // namespace gal::backend