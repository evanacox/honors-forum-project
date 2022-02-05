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
#include "../../utility/flags.h"
#include "../name_resolver.h"
#include "../type_checker.h"
#include "absl/strings/match.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_replace.h"
#include "builtins.h"
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
#include <algorithm>

namespace ast = gal::ast;

namespace {
  struct IntegralInfo {
    std::int32_t width;
    bool is_signed;
  };

  bool should_generate_panics() noexcept {
    return gal::flags().opt() == gal::OptLevel::none && !gal::flags().no_checking();
  }

  IntegralInfo integral_info(gal::backend::ConstantPool* pool, const ast::Type& type) noexcept {
    switch (type.type()) {
      case ast::TypeType::builtin_integral: {
        auto& i = gal::as<ast::BuiltinIntegralType>(type);

        if (auto width = ast::width_of(i.width())) {
          return {static_cast<std::int32_t>(*width), i.has_sign()};
        } else {
          return {static_cast<std::int32_t>(pool->native_type()->getIntegerBitWidth()), i.has_sign()};
        }
      }
      case ast::TypeType::builtin_bool: return {1, false};
      case ast::TypeType::builtin_char:
      case ast::TypeType::builtin_byte: return {8, false};
      default: assert(false); break;
    }

    return IntegralInfo{};
  }

#if !defined(NDEBUG) && defined(__GNUC__)
  // can't actually just print IR from gdb, need this to exist to call from GDB
  __attribute__((unused)) void print_ir(gal::backend::LLVMState* state) {
    state->module()->print(llvm::outs(), nullptr);
  }
#endif
} // namespace

namespace gal::backend {
  CodeGenerator::CodeGenerator(llvm::LLVMContext* context,
      const ast::Program& program,
      const llvm::TargetMachine& machine) noexcept
      : program_{program},
        state_{context, machine, program},
        pool_{&state_},
        variables_{state_.builder(), state_.layout()} {}

  std::unique_ptr<llvm::Module> CodeGenerator::codegen() noexcept {
    // everything besides functions can be defined right now,
    // but functions are just declared, so we can call them later
    for (auto& decl : program_.decls()) {
      if (decl->is(ast::DeclType::fn_decl)) {
        auto& fn = gal::as<ast::FnDeclaration>(*decl);

        (void)codegen_proto(fn.proto(), fn.mangled_name());
      } else {
        decl->accept(this);
      }
    }

    // generate the builtins after everything else, to avoid polluting top of the file
    backend::generate_builtins(&state_);

    // we now go back and actually codegen for each function now that it's safe to generate calls
    for (auto& decl : program_.decls()) {
      if (decl->is(ast::DeclType::fn_decl)) {
        // cast isn't strictly necessary, but it allows devirtualizing the `accept` call. let's
        // micro-optimize for no reason!
        auto& fn = gal::as<ast::FnDeclaration>(*decl);

        fn.accept(this);

        // why is it *true* on error instead of false on error? the verbiage is completely backwards
        if (llvm::verifyFunction(*state_.module()->getFunction(fn.mangled_name()), &llvm::errs())) {
          state_.module()->print(llvm::outs(), nullptr);

          assert(false);
        }
      }
    }

    return state_.take_module();
  }

  llvm::Function* CodeGenerator::codegen_proto(const ast::FnPrototype& proto, std::string_view name) noexcept {
    if (auto* ptr = state_.module()->getFunction(name); ptr != nullptr) {
      return ptr;
    }

    auto arg_types = std::vector<llvm::Type*>{};

    for (auto& arg : proto.args()) {
      arg_types.push_back(pool_.map_type(arg.type()));
    }

    auto* function_type = llvm::FunctionType::get(pool_.map_type(proto.return_type()), arg_types, false);
    auto* fn =
        llvm::Function::Create(function_type, llvm::Function::ExternalLinkage, llvm::Twine(name), state_.module());

    fn->setDSOLocal(true);

    auto i = 0u;
    for (auto& arg : proto.args()) {
      // references can only be (legally) made from valid objects,
      // these are valid. it's UB to have a null/invalid reference
      if (arg.type().is(ast::TypeType::reference)) {
        auto& ref = gal::as<ast::ReferenceType>(arg.type());
        auto list = llvm::AttrBuilder();

        list.addDereferenceableAttr(state_.layout().getTypeAllocSize(pool_.map_type(ref.referenced())));
        list.addAttribute(llvm::Attribute::NonNull);

        // 0th index = *function* attributes, 1 is first param
        fn->addAttributes(i + 1, list);
      }

      ++i;
    }

    // no Gallium functions unwind, and if any external functions
    // try to unwind into Gallium code it's UB anyway
    fn->setDoesNotThrow();

    for (auto& attribute : proto.attributes()) {
      switch (attribute.type) {
        case ast::AttributeType::builtin_pure: fn->addFnAttr(llvm::Attribute::ReadOnly); break;
        case ast::AttributeType::builtin_throws: assert(false); break;
        case ast::AttributeType::builtin_always_inline: fn->addFnAttr(llvm::Attribute::AlwaysInline); break;
        case ast::AttributeType::builtin_inline: fn->addFnAttr(llvm::Attribute::InlineHint); break;
        case ast::AttributeType::builtin_no_inline: fn->addFnAttr(llvm::Attribute::NoInline); break;
        case ast::AttributeType::builtin_malloc:
          fn->addAttribute(llvm::AttributeList::ReturnIndex, llvm::Attribute::NoAlias);
          break;
        case ast::AttributeType::builtin_hot: fn->addFnAttr(llvm::Attribute::Hot); break;
        case ast::AttributeType::builtin_cold: fn->addFnAttr(llvm::Attribute::Cold); break;
        case ast::AttributeType::builtin_arch: assert(false); break;
        case ast::AttributeType::builtin_noreturn: fn->setDoesNotReturn(); break;
        case ast::AttributeType::builtin_stdlib: fn->setLinkage(llvm::Function::LinkOnceODRLinkage); break;
      }
    }

    return fn;
  }

  void CodeGenerator::visit(const ast::ImportDeclaration&) {}

  void CodeGenerator::visit(const ast::ImportFromDeclaration&) {}

  void CodeGenerator::visit(const ast::FnDeclaration& declaration) {
    reset_fn_state();
    auto is_void = declaration.proto().return_type().is(ast::TypeType::builtin_void);
    auto* fn = codegen_proto(declaration.proto(), declaration.mangled_name());

    // need to update current_fn() before we can use `create_block` anywhere else
    auto* entry = llvm::BasicBlock::Create(state_.context(), "entry", fn);
    builder()->SetInsertPoint(entry);

    // now we're safe to use this since the insert point is inside `fn`
    exit_block_ = create_block("exit", true);
    dead_block_ = create_block("__to_delete", true);

    if (!is_void) {
      auto* type = pool_.map_type(declaration.proto().return_type());
      return_value_ = builder()->CreateAlloca(type);
      builder()->SetInsertPoint(exit_block_);
      builder()->CreateRet(builder()->CreateLoad(type, return_value_));
    } else {
      builder()->SetInsertPoint(exit_block_);
      builder()->CreateRetVoid();
    }

    builder()->SetInsertPoint(entry);

    variables_.enter_scope();

    {
      auto fn_args = declaration.proto().args();
      auto it = fn_args.begin();

      // copy all args onto stack, so we don't have to special-case when trying to extract from params or whatever
      for (auto& arg : fn->args()) {
        auto* alloca = builder()->CreateAlloca(arg.getType());
        variables_.set((it++)->name(), alloca);
        builder()->CreateStore(&arg, alloca);
      }
    }

    auto last_expr = codegen(declaration.body());

    if (!is_void && last_expr != nullptr) { // returns and similar will give `nullptr`, ignore them
      builder()->CreateStore(last_expr, return_value_);
    }

    variables_.leave_scope();
    builder()->CreateBr(exit_block_);
    dead_block_->eraseFromParent();
  }

  void CodeGenerator::visit(const ast::StructDeclaration&) {}

  void CodeGenerator::visit(const ast::ClassDeclaration&) {}

  void CodeGenerator::visit(const ast::TypeDeclaration&) {}

  void CodeGenerator::visit(const ast::MethodDeclaration&) {}

  void CodeGenerator::visit(const ast::ExternalFnDeclaration& declaration) {
    // `__builtin` functions may or may not exist at the IR level
    if (!absl::StartsWith(declaration.mangled_name(), "__builtin")) {
      (void)codegen_proto(declaration.proto(), declaration.mangled_name());
    }
  }

  void CodeGenerator::visit(const ast::ExternalDeclaration& declaration) {
    for (auto& fn : declaration.externals()) {
      fn->accept(this);
    }
  }

  void CodeGenerator::visit(const ast::ConstantDeclaration& declaration) {
    auto* type = pool_.map_type(declaration.hint());
    auto* initializer = pool_.constant(type, declaration.initializer());
    auto* constant = state_.module()->getOrInsertGlobal(declaration.mangled_name(), type);
    auto* global = llvm::cast<llvm::GlobalVariable>(constant);
    global->setInitializer(initializer);
    global->setConstant(true);
  }

  void CodeGenerator::visit(const ast::StringLiteralExpression& expression) {
    auto* type = pool_.slice_of(llvm::Type::getInt8Ty(state_.context()));
    auto* literal = pool_.string_literal(expression.text_unquoted());
    auto* pointer_to_first = builder()->CreateInBoundsGEP(
        pool_.array_of(llvm::Type::getInt8Ty(state_.context()), expression.text_unquoted().size() + 1),
        literal,
        {pool_.constant64(0), pool_.constant64(0)});

    auto* insert_1 = builder()->CreateInsertValue(llvm::UndefValue::get(type), pointer_to_first, {0});
    auto* insert_2 = builder()->CreateInsertValue(insert_1,
        pool_.constant64(static_cast<std::int64_t>(expression.text_unquoted().size())),
        {1});

    Expr::return_value(insert_2);
  }

  void CodeGenerator::visit(const ast::IntegerLiteralExpression& expression) {
    auto* type = pool_.map_type(expression.result());

    Expr::return_value(pool_.constant(type, expression));
  }

  void CodeGenerator::visit(const ast::FloatLiteralExpression& expression) {
    auto* type = pool_.map_type(expression.result());

    Expr::return_value(pool_.constant(type, expression));
  }

  void CodeGenerator::visit(const ast::BoolLiteralExpression& expression) {
    Expr::return_value(pool_.constant(llvm::Type::getInt1Ty(state_.context()), expression));
  }

  void CodeGenerator::visit(const ast::CharLiteralExpression& expression) {
    Expr::return_value(pool_.constant(llvm::Type::getInt8Ty(state_.context()), expression));
  }

  void CodeGenerator::visit(const ast::NilLiteralExpression& expression) {
    Expr::return_value(pool_.constant(llvm::Type::getInt8PtrTy(state_.context()), expression));
  }

  void CodeGenerator::visit(const ast::ArrayExpression& expression) {
    auto* type = pool_.map_type(expression.result());
    auto* alloca = builder()->CreateAlloca(type);
    auto count = 0;

    for (const auto& expr : expression.elements()) {
      auto val = codegen_promoting(*expr);
      auto* ptr = builder()->CreateInBoundsGEP(type,
          alloca,
          {llvm::ConstantInt::get(llvm::Type::getInt64Ty(state_.context()), 0),
              llvm::ConstantInt::get(llvm::Type::getInt64Ty(state_.context()), count++)});

      builder()->CreateStore(val, ptr);
    }

    Expr::return_value(StoredValue{alloca, StorageLoc::mem});
  }

  void CodeGenerator::visit(const ast::UnqualifiedIdentifierExpression&) {
    assert(false);
  }

  void CodeGenerator::visit(const ast::IdentifierExpression&) {
    assert(false);
  }

  void CodeGenerator::visit(const ast::StaticGlobalExpression& expression) {
    auto& decl = gal::as<ast::ConstantDeclaration>(expression.decl());
    auto* type = pool_.map_type(decl.hint());
    auto* global = state_.module()->getGlobalVariable(decl.mangled_name());

    Expr::return_value(builder()->CreateLoad(type, global));
  }

  void CodeGenerator::visit(const ast::LocalIdentifierExpression& expression) {
    Expr::return_value(StoredValue{variables_.get(expression.name()), StorageLoc::mem});
  }

  void CodeGenerator::visit(const ast::StructExpression& expression) {
    auto* type = pool_.map_type(expression.result());
    auto* alloca = builder()->CreateAlloca(type);

    for (auto& field : expression.fields()) {
      auto initializer = codegen_promoting(field.init());
      auto* gep = builder()->CreateInBoundsGEP(type,
          alloca,
          {pool_.constant64(0),
              pool_.constant32(static_cast<std::int32_t>(
                  pool_.field_index(gal::as<ast::UserDefinedType>(expression.result()), field.name())))});

      builder()->CreateStore(initializer, gep);
    }

    Expr::return_value(StoredValue{alloca, StorageLoc::mem});
  }

  void CodeGenerator::visit(const ast::CallExpression& expression) {
    auto* return_type = pool_.map_type(expression.result());
    auto* fn_type = pool_.map_type(expression.callee().result());
    auto callee = codegen_promoting(expression.callee(), fn_type);
    auto args = llvm::SmallVector<llvm::Value*, 8>{};

    for (auto& arg : expression.args()) {
      args.push_back(codegen_promoting(*arg));
    }

    auto* call = builder()->CreateCall(llvm::cast<llvm::FunctionType>(fn_type), callee, args);

    if (expression.result().need_address()) {
      auto* alloca = builder()->CreateAlloca(return_type);
      builder()->CreateStore(call, alloca);

      return Expr::return_value(StoredValue{alloca, StorageLoc::mem});
    }

    return Expr::return_value(call);
  }

  void CodeGenerator::visit(const ast::StaticCallExpression& expression) {
    auto args_types = llvm::SmallVector<llvm::Type*, 8>{};
    auto args = llvm::SmallVector<llvm::Value*, 8>{};

    for (auto& expr : expression.args()) {
      auto* type = pool_.map_type(expr->result());
      args_types.push_back(type);
      args.push_back(codegen_promoting(*expr, type));
    }

    auto name = std::visit(
        [](auto* decl) {
          return decl->mangled_name();
        },
        expression.callee().decl());

    // need to handle builtins, they will all be static-call exprs
    if (absl::StartsWith(name, "__builtin")) {
      return Expr::return_value(backend::call_builtin(name, &state_, args));
    }

    auto* fn = state_.module()->getFunction(name);
    auto* call = builder()->CreateCall(fn, args);

    if (expression.result().need_address()) {
      auto* alloca = builder()->CreateAlloca(pool_.map_type(expression.result()));
      builder()->CreateStore(call, alloca);

      return Expr::return_value(StoredValue{alloca, StorageLoc::mem});
    }

    Expr::return_value(call);
  }

  void CodeGenerator::visit(const ast::MethodCallExpression&) {}

  void CodeGenerator::visit(const ast::StaticMethodCallExpression&) {}

  void CodeGenerator::visit(const ast::IndexExpression& expression) {
    auto& result_type = gal::as<ast::IndirectionType>(expression.result()).produced();
    auto offset = codegen_promoting(*expression.indices()[0]); // TODO: multiple indices?
    auto* array_ptr = static_cast<llvm::Value*>(nullptr);
    auto* array_element_type = pool_.map_type(result_type);

    // we either have a slice in a register or we have an array sitting in memory
    if (expression.callee().result().is(ast::TypeType::slice)) {
      auto array = codegen_promoting(expression.callee());
      array_ptr = builder()->CreateExtractValue(array, {0});
      auto* size = builder()->CreateExtractValue(array, {1});
      auto* out_of_bounds = builder()->CreateICmpSGE(offset, size);

      panic_if(expression.loc(), out_of_bounds, "tried to access out-of-bounds on slice");
    } else {
      auto array = codegen(expression.callee());
      array_ptr = builder()->CreateBitCast(array, pool_.pointer_to(array_element_type));
    }

    Expr::return_value(builder()->CreateInBoundsGEP(array_element_type,
        array_ptr,
        std::initializer_list<llvm::Value*>{offset.value()}));
  }

  void CodeGenerator::visit(const ast::FieldAccessExpression& expr) {
    auto value = codegen(expr.object());
    auto* struct_type = pool_.map_type(expr.user_type());

    auto field_idx = pool_.field_index(gal::as<ast::UserDefinedType>(expr.user_type()), expr.field_name());
    auto* gep = builder()->CreateInBoundsGEP(struct_type,
        value,
        {pool_.constant64(0), pool_.constant32(static_cast<std::int32_t>(field_idx))});

    Expr::return_value(gep);
  }

  void CodeGenerator::visit(const ast::GroupExpression& expression) {
    Expr::return_value(codegen(expression.expr()));
  }

  void CodeGenerator::visit(const ast::UnaryExpression& expr) {
    switch (expr.op()) {
      case ast::UnaryOp::ref_to:
      case ast::UnaryOp::mut_ref_to:
        // any of these that are still in the AST just need an address basically.
        // whatever they're attached to are lvalues, so we can just assume they're in memory already
        return Expr::return_value(codegen(expr.expr()));
      case ast::UnaryOp::dereference:
        // we know that anything here will already be at least 1 level of pointer deep,
        // but if the *pointer value* is in memory we need to load it
        return Expr::return_value(codegen_promoting(expr.expr()));

      default: break;
    }

    auto value = codegen_promoting(expr.expr());

    switch (expr.op()) {
      case ast::UnaryOp::bitwise_not:
        // apparently LLVM just made a helper for `xor %thing, -1`, im not complaining
        return Expr::return_value(builder()->CreateNot(value));
      case ast::UnaryOp::logical_not:
        // "logical not" is just bool negation after all! why special-case it
        return Expr::return_value(builder()->CreateNeg(value));
      case ast::UnaryOp::negate: {
        if (should_generate_panics() && expr.result().is_integral()) {
          auto* result = panic_if_overflow(expr.loc(),
              "underflowed while negating",
              llvm::Intrinsic::ssub_with_overflow,
              pool_.zero(value.type()),
              value);

          return Expr::return_value(result);
        }

        if (expr.result().is_integral()) {
          return Expr::return_value(builder()->CreateNSWNeg(value));
        } else {
          return Expr::return_value(builder()->CreateFNeg(value));
        }
      }
      default: assert(false); break;
    }
  }

  llvm::Value* CodeGenerator::generate_mul(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept {
    if (expr.result().is_integral()) {
      auto info = integral_info(&pool_, expr.result());

      if (should_generate_panics() && expr.result().is_integral()) {
        auto intrin = (info.is_signed) ? llvm::Intrinsic::smul_with_overflow : llvm::Intrinsic::umul_with_overflow;

        return panic_if_overflow(expr.loc(), "overflowed in multiplication", intrin, lhs, rhs);
      }

      return (info.is_signed) ? builder()->CreateNSWMul(lhs, rhs) : builder()->CreateNUWMul(lhs, rhs);
    }

    return builder()->CreateFMul(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_div(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept {
    if (should_generate_panics()) {
      // TODO: figure out how to detect division overflow
      // TODO: detect divide by zero?
    }

    if (expr.result().is_integral()) {
      auto info = integral_info(&pool_, expr.result());

      return (info.is_signed) ? builder()->CreateSDiv(lhs, rhs) : builder()->CreateUDiv(lhs, rhs);
    }

    return builder()->CreateFMul(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_mod(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept {
    if (should_generate_panics()) {
      // TODO: figure out how to detect division overflow
      // TODO: detect divide by zero?
    }

    if (expr.result().is_integral()) {
      auto info = integral_info(&pool_, expr.result());

      return (info.is_signed) ? builder()->CreateSRem(lhs, rhs) : builder()->CreateURem(lhs, rhs);
    }

    return builder()->CreateFRem(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_add(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept {
    if (expr.result().is_integral()) {
      auto info = integral_info(&pool_, expr.result());

      if (should_generate_panics() && expr.result().is_integral()) {
        auto intrin = (info.is_signed) ? llvm::Intrinsic::sadd_with_overflow : llvm::Intrinsic::uadd_with_overflow;

        return panic_if_overflow(expr.loc(), "overflowed in addition", intrin, lhs, rhs);
      }

      return (info.is_signed) ? builder()->CreateNSWAdd(lhs, rhs) : builder()->CreateNUWAdd(lhs, rhs);
    }

    return builder()->CreateFAdd(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_sub(const ast::Expression& expr, llvm::Value* lhs, llvm::Value* rhs) noexcept {
    if (expr.result().is_integral()) {
      auto info = integral_info(&pool_, expr.result());

      if (should_generate_panics() && expr.result().is_integral()) {
        auto intrin = (info.is_signed) ? llvm::Intrinsic::ssub_with_overflow : llvm::Intrinsic::usub_with_overflow;

        return panic_if_overflow(expr.loc(), "overflowed in subtraction", intrin, lhs, rhs);
      }

      return (info.is_signed) ? builder()->CreateNSWSub(lhs, rhs) : builder()->CreateNUWSub(lhs, rhs);
    }

    return builder()->CreateFSub(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_left_shift(const ast::Expression& expr,
      llvm::Value* lhs,
      llvm::Value* rhs) noexcept {
    auto info = integral_info(&pool_, expr.result());

    if (should_generate_panics() && expr.result().is_integral()) {
      auto* larger_than = builder()->CreateICmpUGE(rhs, pool_.constant_of(info.width, info.width));

      panic_if(expr.loc(), larger_than, "cannot shift left by number larger than the bit-width of the type");
    }

    return builder()->CreateShl(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_right_shift(const ast::Expression& expr,
      llvm::Value* lhs,
      llvm::Value* rhs) noexcept {
    auto info = integral_info(&pool_, expr.result());

    if (should_generate_panics() && expr.result().is_integral()) {
      auto* larger_than = builder()->CreateICmpUGE(rhs, pool_.constant_of(info.width, info.width));

      panic_if(expr.loc(), larger_than, "cannot shift right by number larger than the bit-width of the type");
    }

    return builder()->CreateAShr(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_bit_and(const ast::Expression&, llvm::Value* lhs, llvm::Value* rhs) noexcept {
    return builder()->CreateAnd(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_bit_or(const ast::Expression&, llvm::Value* lhs, llvm::Value* rhs) noexcept {
    return builder()->CreateOr(lhs, rhs);
  }

  llvm::Value* CodeGenerator::generate_bit_xor(const ast::Expression&, llvm::Value* lhs, llvm::Value* rhs) noexcept {
    return builder()->CreateXor(lhs, rhs);
  }

  void CodeGenerator::visit(const ast::BinaryExpression& expr) {
    if (expr.op() == ast::BinaryOp::assignment || expr.is_compound_assignment()) {
      auto dest = codegen(expr.lhs());
      auto rhs = codegen_promoting(expr.rhs());

      if (expr.op() == ast::BinaryOp::assignment) {
        builder()->CreateStore(rhs, dest);

        return Expr::return_value(nullptr);
      }

      // we're only supposed to evaluate the lhs once!
      auto* lhs = builder()->CreateLoad(pool_.map_type(expr.lhs().result()), dest);
      auto* final_value = static_cast<llvm::Value*>(nullptr);

      switch (expr.op()) {
        case ast::BinaryOp::add_eq: final_value = generate_add(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::sub_eq: final_value = generate_sub(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::mul_eq: final_value = generate_mul(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::div_eq: final_value = generate_div(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::mod_eq: final_value = generate_mod(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::left_shift_eq: final_value = generate_left_shift(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::right_shift_eq: final_value = generate_right_shift(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::bitwise_and_eq: final_value = generate_bit_and(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::bitwise_or_eq: final_value = generate_bit_or(expr.rhs(), lhs, rhs); break;
        case ast::BinaryOp::bitwise_xor_eq: final_value = generate_bit_xor(expr.rhs(), lhs, rhs); break;
        default: assert(false);
      }

      builder()->CreateStore(final_value, dest);

      return Expr::return_value(nullptr);
    }

    auto lhs = codegen_promoting(expr.lhs());
    auto rhs = codegen_promoting(expr.rhs());

    if (expr.is_ordering()) {
      auto info = integral_info(&pool_, expr.lhs().result());

      switch (expr.op()) {
        case ast::BinaryOp::lt:
          return Expr::return_value(
              (info.is_signed) ? builder()->CreateICmpSLT(lhs, rhs) : builder()->CreateICmpULT(lhs, rhs));
        case ast::BinaryOp::gt:
          return Expr::return_value(
              (info.is_signed) ? builder()->CreateICmpSGT(lhs, rhs) : builder()->CreateICmpUGT(lhs, rhs));
        case ast::BinaryOp::lt_eq:
          return Expr::return_value(
              (info.is_signed) ? builder()->CreateICmpSLE(lhs, rhs) : builder()->CreateICmpULE(lhs, rhs));
        case ast::BinaryOp::gt_eq:
          return Expr::return_value(
              (info.is_signed) ? builder()->CreateICmpSGE(lhs, rhs) : builder()->CreateICmpUGE(lhs, rhs));
        default: assert(false);
      }
    }

    switch (expr.op()) {
      case ast::BinaryOp::equals: {
        // TODO: support equality for non-integral/non-fp
        if (expr.lhs().result().is_integral()) {
          return Expr::return_value(builder()->CreateICmpEQ(lhs, rhs));
        } else {
          return Expr::return_value(builder()->CreateFCmpOEQ(lhs, rhs));
        }
      }
      case ast::BinaryOp::not_equal: {
        // TODO: see above
        if (expr.lhs().result().is_integral()) {
          return Expr::return_value(builder()->CreateICmpNE(lhs, rhs));
        } else {
          return Expr::return_value(builder()->CreateFCmpONE(lhs, rhs));
        }
      }
      case ast::BinaryOp::logical_and: return Expr::return_value(builder()->CreateAnd(lhs, rhs));
      case ast::BinaryOp::logical_or: return Expr::return_value(builder()->CreateOr(lhs, rhs));
      case ast::BinaryOp::logical_xor: return Expr::return_value(builder()->CreateXor(lhs, rhs));
      case ast::BinaryOp::mul: return Expr::return_value(generate_mul(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::div: return Expr::return_value(generate_div(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::mod: return Expr::return_value(generate_mod(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::add: return Expr::return_value(generate_add(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::sub: return Expr::return_value(generate_sub(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::left_shift: return Expr::return_value(generate_left_shift(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::right_shift: return Expr::return_value(generate_right_shift(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::bitwise_and: return Expr::return_value(generate_bit_and(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::bitwise_or: return Expr::return_value(generate_bit_or(expr.lhs(), lhs, rhs));
      case ast::BinaryOp::bitwise_xor: return Expr::return_value(generate_bit_xor(expr.lhs(), lhs, rhs));
      default: assert(false);
    }
  }

  void CodeGenerator::visit(const ast::CastExpression& expr) {
    auto value = codegen_promoting(expr.castee());

    if (expr.cast_to().is_integral() && expr.castee().result().is_integral()) {
      auto to_info = integral_info(&pool_, expr.cast_to());
      auto from_info = integral_info(&pool_, expr.castee().result());
      auto* cast = integer_cast(to_info.width, from_info.width, from_info.is_signed, value);

      return Expr::return_value(cast);
    }

    if (expr.cast_to().is_integral() && expr.castee().result().is(ast::TypeType::builtin_float)) {
      auto info = integral_info(&pool_, expr.cast_to());

      // TODO: implement checking for safety in debug mode
      // TODO: if (T::MIN as FloatType - epsilon) <= x <= (T::MAX as FloatType + epsilon), we're ok
      // TODO: if not, we need to panic because LLVM outputs a poison
      auto* cast = (info.is_signed) ? builder()->CreateFPToSI(value, pool_.map_type(expr.cast_to()))
                                    : builder()->CreateFPToUI(value, pool_.map_type(expr.cast_to()));

      return Expr::return_value(cast);
    }

    if (expr.cast_to().is(ast::TypeType::builtin_float) && expr.castee().result().is_integral()) {
      auto info = integral_info(&pool_, expr.castee().result());

      auto* cast = (info.is_signed) ? builder()->CreateSIToFP(value, pool_.map_type(expr.cast_to()))
                                    : builder()->CreateUIToFP(value, pool_.map_type(expr.cast_to()));

      return Expr::return_value(cast);
    }

    // other casts are just bitcasts effectively, the only real concerns are with types
    auto* cast = builder()->CreateBitCast(value, pool_.map_type(expr.cast_to()));

    return Expr::return_value(cast);
  }

  void CodeGenerator::visit(const ast::IfThenExpression& expr) {
    auto is_void = expr.result().is(ast::TypeType::builtin_void);
    auto cond = codegen_promoting(expr.condition());
    auto* type = pool_.map_type(expr.result());

    auto* true_branch = create_block();
    auto* false_branch = create_block();
    auto* merge = create_block();
    builder()->CreateCondBr(cond, true_branch, false_branch);

    builder()->SetInsertPoint(true_branch);
    auto true_value = codegen_promoting(expr.true_branch());
    builder()->CreateBr(merge);

    builder()->SetInsertPoint(false_branch);
    auto false_value = codegen_promoting(expr.false_branch());
    builder()->CreateBr(merge);

    merge_with(merge);

    if (is_void) {
      return Expr::return_value(nullptr);
    }

    auto* phi = builder()->CreatePHI(type, 2);
    phi->addIncoming(true_value, true_branch);
    phi->addIncoming(false_value, false_branch);

    return Expr::return_value(phi);
  }

  void CodeGenerator::visit(const ast::IfElseExpression& expr) {
    auto* type = pool_.map_type(expr.result());
    auto evaluable = expr.is_evaluable() && !expr.result().is(ast::TypeType::builtin_void);
    auto* result_store = (evaluable) ? builder()->CreateAlloca(type) : nullptr;

    // we effectively just need to turn `if { ... } elif { ... } else { ... }`
    // into `if { ... } else { if { .... } else { ... } }`
    //
    // we do that by repeatedly updating `if_block` and `else_block`
    auto* if_block = create_block();
    auto* else_block = create_block();
    auto* merge = create_block();
    auto cond = codegen_promoting(expr.condition());
    builder()->CreateCondBr(cond, if_block, else_block);

    auto f = [&, this](llvm::BasicBlock* block, const ast::BlockExpression& block_expr) {
      builder()->SetInsertPoint(block);
      auto result = codegen_promoting(block_expr);

      if (evaluable) {
        builder()->CreateStore(result, result_store);
      }

      builder()->CreateBr(merge);
    };

    f(if_block, expr.block());

    for (auto& elif : expr.elif_blocks()) {
      builder()->SetInsertPoint(else_block);
      auto elif_cond = codegen_promoting(elif.condition());
      auto* elif_block = create_block();
      else_block = create_block();

      builder()->CreateCondBr(elif_cond, elif_block, else_block);

      f(elif_block, elif.block());
    }

    if (auto else_expr = expr.else_block()) {
      f(else_block, **else_expr);
    } else {
      else_block->eraseFromParent();
    }

    merge_with(merge);

    Expr::return_value((evaluable) ? builder()->CreateLoad(type, result_store) : nullptr);
  }

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

  void CodeGenerator::visit(const ast::LoopExpression& expr) {
    // we need to avoid allocating memory in the loop header, so we hoist this
    // outside of the loop header and then jump to the loop header transparently
    if (!expr.result().is(ast::TypeType::builtin_void)) {
      loop_break_value_ = builder()->CreateAlloca(pool_.map_type(expr.result()));
    }

    // since there's never a condition for `loop` expressions, may as well
    // make loop start the body itself and skip the empty loop header block entirely
    loop_start_ = create_block();
    loop_merge_ = create_block();

    builder()->CreateBr(loop_start_);
    builder()->SetInsertPoint(loop_start_);

    // don't care about the last expression
    expr.body().accept(this);
    builder()->CreateBr(loop_start_);

    // loop is done, back to normal.
    merge_with(loop_merge_);

    Expr::return_value(expr.result().is(ast::TypeType::builtin_void) ? nullptr : loop_break_value_);
  }

  void CodeGenerator::visit(const ast::WhileExpression& expr) {
    loop_start_ = create_block();
    auto* loop_body = create_block();
    loop_merge_ = create_block();

    builder()->CreateBr(loop_start_);
    builder()->SetInsertPoint(loop_start_);
    auto cond = codegen_promoting(expr.condition());
    builder()->CreateCondBr(cond, loop_body, loop_merge_);

    builder()->SetInsertPoint(loop_body);
    expr.body().accept(this);
    builder()->CreateBr(loop_start_);

    merge_with(loop_merge_);

    Expr::return_value(nullptr);
  }

  void CodeGenerator::visit(const ast::ForExpression& expr) {
    auto* loop_header = create_block();
    loop_start_ = create_block();
    auto* loop_body = create_block();
    loop_merge_ = create_block();

    auto start = codegen_promoting(expr.init());
    auto last = codegen_promoting(expr.last());

    builder()->CreateBr(loop_header);
    builder()->SetInsertPoint(loop_header);

    // we don't want to allocate every loop iteration
    // while we could use a phi here, the object needs to be in memory
    // for the variable resolver to work correctly
    auto value = builder()->CreateAlloca(start.type());
    builder()->CreateStore(start, value);
    variables_.enter_scope();
    variables_.set(expr.loop_variable(), value);
    builder()->CreateBr(loop_start_);
    builder()->SetInsertPoint(loop_start_);

    // actual loop header, just loads current value and compares it
    auto* load = builder()->CreateLoad(start.type(), value);
    auto* cond = builder()->CreateICmpNE(load, last);
    builder()->CreateCondBr(cond, loop_body, loop_merge_);

    builder()->SetInsertPoint(loop_body);
    expr.body().accept(this);

    auto width = start.type()->getIntegerBitWidth();
    auto next = (expr.loop_direction() == ast::ForDirection::up_to)
                    ? generate_add(expr.init(), load, pool_.constant_of(width, 1))
                    : generate_sub(expr.init(), load, pool_.constant_of(width, 1));

    builder()->CreateStore(next, value);
    builder()->CreateBr(loop_start_);
    variables_.leave_scope();

    merge_with(loop_merge_);
    Expr::return_value(nullptr);
  }

  void CodeGenerator::visit(const ast::ReturnExpression& expression) {
    if (auto ptr = expression.value()) {
      auto value = codegen_promoting(**ptr);

      builder()->CreateStore(value, return_value_);
    }

    builder()->CreateBr(exit_block_);
    emit_terminator();

    Expr::return_value(nullptr); // shouldn't be possible to actually *use* this
  }

  void CodeGenerator::visit(const ast::BreakExpression& expr) {
    if (auto value = expr.value()) {
      auto break_value = codegen_promoting(**value);

      builder()->CreateStore(break_value, loop_break_value_);
    }

    builder()->CreateBr(loop_merge_);
    emit_terminator();

    Expr::return_value(nullptr);
  }

  void CodeGenerator::visit(const ast::ContinueExpression&) {
    builder()->CreateBr(loop_start_);
    emit_terminator();

    Expr::return_value(nullptr);
  }

  void CodeGenerator::visit(const ast::ImplicitConversionExpression& expr) {
    auto value =
        (expr.expr().is(ast::ExprType::address_of)) // fix bug where variable would get promoted and codegen would break
            ? codegen(expr.expr())
            : codegen_promoting(expr.expr());

    // integer literals get implicitly converted into compatible integer types
    if (expr.expr().result().is(ast::TypeType::unsized_integer)) {
      auto result_info = integral_info(&pool_, expr.cast_to());
      auto* cast = integer_cast(result_info.width, value.type()->getIntegerBitWidth(), false, value);

      return Expr::return_value(cast);
    }

    if (expr.expr().result().is(ast::TypeType::reference) && expr.cast_to().is(ast::TypeType::slice)) {
      auto& ref = gal::as<ast::ReferenceType>(expr.expr().result());
      auto& array = gal::as<ast::ArrayType>(ref.referenced());
      auto* slice_type = pool_.slice_of(pool_.map_type(array.element_type()));
      auto* data =
          builder()->CreateInBoundsGEP(pool_.map_type(array), value, {pool_.constant64(0), pool_.constant64(0)});
      auto* insert = builder()->CreateInsertValue(llvm::UndefValue::get(slice_type), data, {0});

      return Expr::return_value(builder()->CreateInsertValue(insert,
          pool_.constant_of(pool_.native_type()->getIntegerBitWidth(), static_cast<std::int64_t>(array.size())),
          {1}));
    }

    Expr::return_value(builder()->CreateBitCast(value, pool_.map_type(expr.cast_to())));
  }

  void CodeGenerator::visit(const ast::LoadExpression& expr) {
    // don't care what it is, front-end says we should load, so it's fine
    auto ptr = codegen_promoting(expr.expr());
    auto* type = pool_.map_type(expr.result());

    Expr::return_value(builder()->CreateLoad(type, ptr));
  }

  void CodeGenerator::visit(const ast::AddressOfExpression& expr) {
    auto value = codegen(expr.expr());

    Expr::return_value(value); // anything we take & of will have an address
  }

  void CodeGenerator::visit(const ast::BindingStatement& statement) {
    auto value = (statement.initializer().result().is_one_of(ast::TypeType::pointer, ast::TypeType::reference))
                     ? codegen(statement.initializer())
                     : codegen_promoting(statement.initializer());

    auto* alloca = builder()->CreateAlloca(value.type());
    builder()->CreateStore(value, alloca);

    variables_.set(statement.name(), alloca);

    Stmt::return_value(nullptr);
  }

  void CodeGenerator::visit(const ast::ExpressionStatement& statement) {
    Stmt::return_value(codegen_promoting(statement.expr()));
  }

  void CodeGenerator::visit(const ast::AssertStatement& statement) {
    auto cond = codegen_promoting(statement.assertion());

    if (builder()->GetInsertBlock() != dead_block_) {
      auto* merge = create_block();
      auto* assert_fail = assert_block();

      assert_phi_->addIncoming(source_loc(statement.loc(), statement.message().text_unquoted()),
          builder()->GetInsertBlock());

      builder()->CreateCondBr(cond, merge, assert_fail);
      builder()->SetInsertPoint(merge);
    }

    Stmt::return_value(nullptr);
  }

  backend::StoredValue CodeGenerator::codegen(const ast::Expression& expr) noexcept {
    return expr.accept(this);
  }

  backend::StoredValue CodeGenerator::codegen_promoting(const ast::Expression& expr, llvm::Type* type) noexcept {
    auto inst = codegen(expr);

    // if it's not a register value, create a load to it and return that
    if (inst != nullptr && inst.loc() == StorageLoc::mem) {
      // allow lazy generation if we don't already have the type
      return builder()->CreateLoad((type == nullptr) ? pool_.map_type(expr.result()) : type, inst);
    }

    return inst;
  }

  llvm::IRBuilder<>* CodeGenerator::builder() noexcept {
    return state_.builder();
  }

  llvm::Function* CodeGenerator::current_fn() noexcept {
    return builder()->GetInsertBlock()->getParent();
  }

  std::string CodeGenerator::next_label() noexcept {
    return absl::StrCat(".bb", gal::to_digits(curr_label_++));
  }

  void CodeGenerator::reset_fn_state() noexcept {
    curr_label_ = 1;
    loop_start_ = nullptr;
    loop_merge_ = nullptr;
    exit_block_ = nullptr;
    dead_block_ = nullptr;
    panic_block_ = nullptr;
    panic_phi_ = nullptr;
    return_value_ = nullptr;
    loop_break_value_ = nullptr;
  }

  void CodeGenerator::emit_terminator() noexcept {
    builder()->SetInsertPoint(dead_block_);
  }

  llvm::BasicBlock* CodeGenerator::panic_block() noexcept {
    if (panic_block_ == nullptr) {
      auto* current_bb = builder()->GetInsertBlock();

      panic_block_ = llvm::BasicBlock::Create(state_.context(), "panic", current_fn());
      builder()->SetInsertPoint(panic_block_);
      panic_phi_ = builder()->CreatePHI(pool_.source_info_type(), 0);

      auto* file = builder()->CreateExtractValue(panic_phi_, {0});
      auto* line = builder()->CreateExtractValue(panic_phi_, {1});
      auto* msg = builder()->CreateExtractValue(panic_phi_, {2});

      builder()->CreateCall(state_.module()->getFunction("__gallium_panic"), {file, line, msg});
      builder()->CreateUnreachable();

      builder()->SetInsertPoint(current_bb);
    }

    return panic_block_;
  }

  llvm::BasicBlock* CodeGenerator::assert_block() noexcept {
    if (assert_block_ == nullptr) {
      auto* current_bb = builder()->GetInsertBlock();

      assert_block_ = llvm::BasicBlock::Create(state_.context(), "assert_fail", current_fn());
      builder()->SetInsertPoint(assert_block_);
      assert_phi_ = builder()->CreatePHI(pool_.source_info_type(), 0);

      auto* file = builder()->CreateExtractValue(assert_phi_, {0});
      auto* line = builder()->CreateExtractValue(assert_phi_, {1});
      auto* msg = builder()->CreateExtractValue(assert_phi_, {2});

      builder()->CreateCall(state_.module()->getFunction("__gallium_assert_fail"), {file, line, msg});
      builder()->CreateUnreachable();

      builder()->SetInsertPoint(current_bb);
    }

    return assert_block_;
  }

  llvm::Value* CodeGenerator::integer_cast(std::uint32_t to, std::uint32_t from, bool sign, llvm::Value* val) noexcept {
    // no need to generate code to cast in this case, they're literally already
    // both the right type
    if (to == from) {
      return val;
    }

    auto* type = pool_.integer_of_width(to);

    return (sign) ? builder()->CreateSExtOrTrunc(val, type) //
                  : builder()->CreateZExtOrTrunc(val, type);
  }

  llvm::Value* CodeGenerator::panic_if_overflow(const ast::SourceLoc& loc,
      std::string_view message,
      llvm::Intrinsic::IndependentIntrinsics intrin,
      llvm::Value* lhs,
      llvm::Value* rhs) noexcept {
    auto* with_overflow = llvm::Intrinsic::getDeclaration(state_.module(), intrin, {lhs->getType()});
    auto* overflow_result = builder()->CreateCall(with_overflow, {lhs, rhs});

    // overflow_result -> { T, i1 }
    auto* value = builder()->CreateExtractValue(overflow_result, {0});
    auto* did_overflow = builder()->CreateExtractValue(overflow_result, {1});

    panic_if(loc, did_overflow, message);

    return value;
  }

  void CodeGenerator::panic_if(const ast::SourceLoc& loc, llvm::Value* cond, std::string_view message) noexcept {
    // hack, if we returned before ending up down here we may end up generating
    // a phi incoming from dead_block_, and it will later get nuked and screw up the phi
    if (builder()->GetInsertBlock() != dead_block_) {
      auto* merge = create_block();
      auto* panic = panic_block();

      panic_phi_->addIncoming(source_loc(loc, message), builder()->GetInsertBlock());

      builder()->CreateCondBr(cond, panic, merge);
      builder()->SetInsertPoint(merge);
    }
  }

  llvm::Value* CodeGenerator::source_loc(const ast::SourceLoc& loc, std::string_view message) noexcept {
    auto* file = pool_.c_string_literal(loc.file().string());
    auto* line = pool_.constant64(static_cast<std::int64_t>(loc.line()));
    auto* msg = pool_.c_string_literal(message);
    auto* type = pool_.source_info_type();

    auto* insert_1 = builder()->CreateInsertValue(llvm::UndefValue::get(type), file, {0});
    auto* insert_2 = builder()->CreateInsertValue(insert_1, line, {1});
    auto* insert_3 = builder()->CreateInsertValue(insert_2, msg, {2});

    return insert_3;
  }

  llvm::BasicBlock* CodeGenerator::create_block(std::string_view name, bool true_end) noexcept {
    auto* previous_bb = (true_end) ? nullptr : exit_block_;
    auto label = (name.empty()) ? next_label() : std::string{name};

    return llvm::BasicBlock::Create(state_.context(), label, current_fn(), previous_bb);
  }

  void CodeGenerator::merge_with(llvm::BasicBlock* block) noexcept {
    block->moveBefore(exit_block_);
    builder()->SetInsertPoint(block);
  }
} // namespace gal::backend