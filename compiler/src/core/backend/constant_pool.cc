//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./constant_pool.h"
#include "absl/strings/str_replace.h"

namespace ast = gal::ast;

namespace {
  class IntoConstant final : public ast::ConstExpressionVisitor<llvm::Constant*> {
  public:
    explicit IntoConstant(gal::backend::ConstantPool* gen, llvm::Type* type) noexcept : type_{type}, gen_{gen} {}

    void visit(const ast::StringLiteralExpression& expr) final {
      return_value(gen_->string_literal(expr.text_unquoted()));
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
    gal::backend::ConstantPool* gen_;
  };
} // namespace

namespace gal::backend {
  using TypeNamePair = std::pair<llvm::Type*, std::string_view>;

  ConstantPool::ConstantPool(LLVMState* state) noexcept : state_{state} {}

  llvm::Constant* ConstantPool::string_literal(std::string_view data) noexcept {
    if (auto it = string_literals_.find(data); it != string_literals_.end()) {
      return it->second;
    }

    auto* type = array_of(integer_of_width(8), data.size() + 1);
    auto* constant = state_->module()->getOrInsertGlobal(absl::StrCat(".str.", curr_str_++), type);
    auto* global = llvm::cast<llvm::GlobalVariable>(constant);
    global->setConstant(true);
    global->setLinkage(llvm::GlobalValue::InternalLinkage); // string literals are not visible outside the module
    global->setInitializer(llvm::ConstantDataArray::getString(state_->context(), data));

    string_literals_.emplace(std::string{data}, constant);

    return constant;
  }

  llvm::Type* ConstantPool::map_type(const ast::Type& type) noexcept {
    return type.accept(this);
  }

  llvm::Constant* ConstantPool::constant64(std::int64_t value) noexcept {
    return constant_of(64, value);
  }

  llvm::Constant* ConstantPool::constant32(std::int32_t value) noexcept {
    return constant_of(32, value);
  }

  llvm::Constant* ConstantPool::constant_of(std::int64_t width, std::int64_t value) noexcept {
    return llvm::ConstantInt::get(integer_of_width(width), value);
  }

  llvm::Type* ConstantPool::native_type() noexcept {
    return integer_of_width(state_->layout().getPointerSizeInBits());
  }

  std::uint32_t ConstantPool::field_index(const ast::UserDefinedType& type, std::string_view name) noexcept {
    (void)type.accept(this);

    // default construct and then insert in
    auto& result = field_names_[type.id().as_string()];

    // the index of `name` is the index in the LLVM type of that field
    return static_cast<std::int32_t>(std::distance(result.begin(), std::find(result.begin(), result.end(), name)));
  }

  llvm::SmallVector<std::pair<llvm::Type*, std::string_view>, 8> ConstantPool::from_structure(
      const ast::StructDeclaration& decl) noexcept {
    auto fields = llvm::SmallVector<TypeNamePair, 8>{};

    for (auto& field : decl.fields()) {
      auto* field_type = field.type().accept(this);

      fields.emplace_back(field_type, field.name());
    }

    std::stable_sort(fields.begin(), fields.end(), [this](const TypeNamePair& lhs, const TypeNamePair& rhs) {
      return state_->layout().getTypeAllocSize(lhs.first).getFixedSize()
             < state_->layout().getTypeAllocSize(rhs.first).getFixedSize();
    });

    return fields;
  }

  void ConstantPool::create_user_type_mapping(std::string_view full_name,
      llvm::ArrayRef<std::pair<llvm::Type*, std::string_view>> array) noexcept {
    // default construct a vec, then insert into it
    auto& vec = field_names_[full_name];

    vec.reserve(array.size());

    std::transform(array.begin(), array.end(), std::back_inserter(vec), [](const TypeNamePair& pair) {
      return pair.second;
    });
  }

  llvm::Type* ConstantPool::struct_from(std::string_view name,
      llvm::ArrayRef<std::pair<llvm::Type*, std::string_view>> array) noexcept {
    if (auto* ptr = llvm::StructType::getTypeByName(state_->context(), name)) {
      return ptr;
    }

    auto field_types = llvm::SmallVector<llvm::Type*, 8>{};
    field_types.reserve(array.size());
    std::transform(array.begin(), array.end(), std::back_inserter(field_types), [](const TypeNamePair& pair) {
      return pair.first;
    });

    return llvm::StructType::create(state_->context(), field_types, name);
  }

  llvm::Constant* ConstantPool::constant(llvm::Type* type, const ast::Expression& expr) noexcept {
    auto constant_generator = IntoConstant{this, type};

    return expr.accept(&constant_generator);
  }

  void ConstantPool::visit(const ast::ReferenceType& type) {
    return_value(pointer_to(map_type(type.referenced())));
  }

  void ConstantPool::visit(const ast::SliceType& type) {
    return_value(llvm::StructType::get(state_->context(), {pointer_to(map_type(type.sliced())), native_type()}));
  }

  void ConstantPool::visit(const ast::PointerType& type) {
    return_value(pointer_to(map_type(type.pointed())));
  }

  void ConstantPool::visit(const ast::BuiltinIntegralType& type) {
    if (auto width = ast::width_of(type.width())) {
      return_value(integer_of_width(*width));
    } else {
      return_value(native_type());
    }
  }

  void ConstantPool::visit(const ast::BuiltinFloatType& type) {
    switch (type.width()) {
      case ast::FloatWidth::ieee_single: return return_value(llvm::Type::getFloatTy(state_->context()));
      case ast::FloatWidth::ieee_double: return return_value(llvm::Type::getDoubleTy(state_->context()));
      case ast::FloatWidth::ieee_quadruple: return return_value(llvm::Type::getFP128Ty(state_->context()));
      default: assert(false); return return_value(nullptr);
    }
  }

  void ConstantPool::visit(const ast::BuiltinByteType&) {
    return_value(llvm::Type::getInt8Ty(state_->context()));
  }

  void ConstantPool::visit(const ast::BuiltinBoolType&) {
    return_value(llvm::Type::getInt1Ty(state_->context()));
  }

  void ConstantPool::visit(const ast::BuiltinCharType&) {
    return_value(llvm::Type::getInt8Ty(state_->context()));
  }

  void ConstantPool::visit(const ast::UnqualifiedUserDefinedType&) {
    assert(false);
  }

  void ConstantPool::visit(const ast::UserDefinedType& type) {
    auto entity = type.id().as_string();

    // either we've already generated an LLVM struct & lookup data, or we need to
    if (auto it = user_types_.find(entity); it != user_types_.end()) {
      return return_value(it->second);
    }

    auto fields = from_structure(gal::as<ast::StructDeclaration>(type.decl()));
    auto* llvm_type = struct_from(absl::StrCat("struct", absl::StrReplaceAll(entity, {{"::", "."}})), fields);

    user_types_.emplace(std::string{entity}, llvm_type);
    create_user_type_mapping(entity, fields);

    return_value(llvm_type);
  }

  void ConstantPool::visit(const ast::FnPointerType& type) {
    auto vec = llvm::SmallVector<llvm::Type*, 8>{};

    for (auto& arg : type.args()) {
      vec.push_back(map_type(*arg));
    }

    return_value(llvm::FunctionType::get(map_type(type.return_type()), vec, false));
  }

  void ConstantPool::visit(const ast::UnqualifiedDynInterfaceType&) {
    assert(false);
  }

  void ConstantPool::visit(const ast::DynInterfaceType&) {
    assert(false);
  }

  void ConstantPool::visit(const ast::VoidType&) {
    return_value(llvm::Type::getVoidTy(state_->context()));
  }

  void ConstantPool::visit(const ast::NilPointerType&) {
    assert(false);
  }

  void ConstantPool::visit(const ast::ErrorType&) {
    assert(false);
  }

  void ConstantPool::visit(const ast::UnsizedIntegerType&) {
    return_value(native_type());
  }

  void ConstantPool::visit(const ast::ArrayType& type) {
    return_value(array_of(map_type(type.element_type()), type.size()));
  }

  void ConstantPool::visit(const ast::IndirectionType& type) {
    return_value(pointer_to(map_type(type.produced())));
  }

  llvm::Type* ConstantPool::pointer_to(llvm::Type* type) noexcept {
    return llvm::PointerType::get(type, state_->layout().getProgramAddressSpace());
  }

  llvm::Type* ConstantPool::slice_of(llvm::Type* type) noexcept {
    return llvm::StructType::get(state_->context(), {pointer_to(type), native_type()});
  }

  llvm::Type* ConstantPool::array_of(llvm::Type* type, std::uint64_t length) noexcept {
    return llvm::ArrayType::get(type, length);
  }

  llvm::Type* ConstantPool::integer_of_width(std::int64_t width) noexcept {
    switch (width) {
      case 1: return llvm::Type::getInt1Ty(state_->context());
      case 8: return llvm::Type::getInt8Ty(state_->context());
      case 16: return llvm::Type::getInt16Ty(state_->context());
      case 32: return llvm::Type::getInt32Ty(state_->context());
      case 64: return llvm::Type::getInt64Ty(state_->context());
      case 128: return llvm::Type::getInt128Ty(state_->context());
      default: assert(false); return nullptr;
    }
  }

  llvm::Type* ConstantPool::source_info_type() noexcept {
    if (auto* ptr = llvm::StructType::getTypeByName(state_->context(), "__GalliumSourceInfo")) {
      return ptr;
    }

    auto* msg_type = llvm::Type::getInt8PtrTy(state_->context());
    auto* line_type = llvm::Type::getInt64Ty(state_->context());

    return llvm::StructType::create(state_->context(), {msg_type, line_type, msg_type}, "__GalliumSourceInfo");
  }

  llvm::Constant* ConstantPool::zero(llvm::Type* type) noexcept {
    return llvm::Constant::getNullValue(type);
  }

  llvm::Value* ConstantPool::c_string_literal(std::string_view data) noexcept {
    auto* lit = string_literal(data);

    return state_->builder()->CreateGEP(array_of(integer_of_width(8), data.size() + 1),
        lit,
        {constant64(0), constant64(0)});
  }
} // namespace gal::backend