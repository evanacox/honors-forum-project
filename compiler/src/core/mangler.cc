//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./mangler.h"
#include "../ast/visitors.h"
#include "../utility/misc.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/str_join.h"
#include "absl/strings/str_split.h"
#include <algorithm>
#include <string>

namespace ast = gal::ast;

namespace {
  class Mangler final : public ast::ConstDeclarationVisitor<std::string>, ast::ConstTypeVisitor<void> {
    using Decl = ast::ConstDeclarationVisitor<std::string>;

  public:
    explicit Mangler() noexcept {
      absl::StrAppend(&builder_, "_G");
    }

    std::string mangle_decl(const ast::Declaration& decl) noexcept {
      return decl.accept(this);
    }

    void visit(const ast::ImportDeclaration&) final {
      assert(false);
    }

    void visit(const ast::ImportFromDeclaration&) final {
      assert(false);
    }

    void visit(const ast::FnDeclaration& declaration) final {
      auto& proto = declaration.proto();

      // extern functions don't get mangled, need to be able to expose over FFI
      if (declaration.external()) {
        return Decl::return_value(std::string{proto.name()});
      }

      build_module_prefix(declaration.id());
      absl::StrAppend(&builder_, "F", proto.name().size(), proto.name());

      auto attributes = proto.attributes();
      auto can_throw = std::any_of(attributes.begin(), attributes.end(), [](const ast::Attribute& attribute) {
        return attribute.type == ast::AttributeType::builtin_throws;
      });

      absl::StrAppend(&builder_, can_throw ? "T" : "N");

      for (auto& arg : proto.args()) {
        mangle(arg.type());
      }

      absl::StrAppend(&builder_, "E");

      mangle(proto.return_type());

      if (builder_ == "_GF4mainNEv") {
        Decl::return_value("__gallium_user_main");
      } else {
        Decl::return_value(std::move(builder_));
      }
    }

    void visit(const ast::StructDeclaration&) final {
      assert(false);
    }

    void visit(const ast::ClassDeclaration&) final {
      assert(false);
    }

    void visit(const ast::TypeDeclaration&) final {
      assert(false);
    }

    void visit(const ast::MethodDeclaration&) final {
      assert(false);
    }

    void visit(const ast::ExternalFnDeclaration& declaration) final {
      // these are not mangled, they're considered "visible" FFI-wise
      Decl::return_value(std::string{declaration.proto().name()});
    }

    void visit(const ast::ExternalDeclaration&) final {
      assert(false);
    }

    void visit(const ast::ConstantDeclaration& declaration) final {
      build_module_prefix(declaration.id());
      absl::StrAppend(&builder_, "C", declaration.name().size(), declaration.name());
      mangle(declaration.hint());

      Decl::return_value(std::move(builder_));
    }

    void visit(const ast::ReferenceType& type) final {
      absl::StrAppend(&builder_, type.mut() ? "S" : "R");

      mangle(type.referenced());
    }

    void visit(const ast::SliceType& type) final {
      absl::StrAppend(&builder_, type.mut() ? "C" : "B");

      mangle(type.sliced());
    }

    void visit(const ast::PointerType& type) final {
      absl::StrAppend(&builder_, type.mut() ? "Q" : "P");

      mangle(type.pointed());
    }

    void visit(const ast::BuiltinIntegralType& type) final {
      using WidthT = std::underlying_type_t<ast::IntegerWidth>;
      static absl::flat_hash_map<std::pair<bool, WidthT>, std::string_view> builtin_lookup{
          {{false, 8}, "d"},
          {{false, 16}, "e"},
          {{false, 32}, "f"},
          {{false, 64}, "g"},
          {{false, 128}, "h"},
          {{false, static_cast<WidthT>(ast::IntegerWidth::native_width)}, "i"},
          {{true, 8}, "j"},
          {{true, 16}, "k"},
          {{true, 32}, "l"},
          {{true, 64}, "m"},
          {{true, 128}, "n"},
          {{true, static_cast<WidthT>(ast::IntegerWidth::native_width)}, "o"},
      };

      absl::StrAppend(&builder_, builtin_lookup.at({type.has_sign(), static_cast<WidthT>(type.width())}));
    }

    void visit(const ast::BuiltinFloatType& type) final {
      switch (type.width()) {
        case ast::FloatWidth::ieee_single: absl::StrAppend(&builder_, "p"); break;
        case ast::FloatWidth::ieee_double: absl::StrAppend(&builder_, "q"); break;
        case ast::FloatWidth::ieee_quadruple: absl::StrAppend(&builder_, "r"); break;
        default: assert(false); break;
      }
    }

    void visit(const ast::BuiltinByteType&) final {
      absl::StrAppend(&builder_, "a");
    }

    void visit(const ast::BuiltinBoolType&) final {
      absl::StrAppend(&builder_, "b");
    }

    void visit(const ast::BuiltinCharType&) final {
      absl::StrAppend(&builder_, "c");
    }

    void visit(const ast::UnqualifiedUserDefinedType&) final {
      assert(false);
    }

    void visit(const ast::UserDefinedType& type) final {
      build_module_prefix(type.id());

      auto name = type.id().name();

      absl::StrAppend(&builder_, "U", name.size(), name);
    }

    void visit(const ast::FnPointerType& type) final {
      absl::StrAppend(&builder_, "FN");

      for (auto& arg : type.args()) {
        mangle(*arg);
      }

      absl::StrAppend(&builder_, "E");

      mangle(type.return_type());
    }

    void visit(const ast::UnqualifiedDynInterfaceType&) final {
      assert(false);
    }

    void visit(const ast::DynInterfaceType& type) final {
      build_module_prefix(type.id());

      auto name = type.id().name();

      absl::StrAppend(&builder_, "D", name.size(), name);
    }

    void visit(const ast::VoidType&) final {
      absl::StrAppend(&builder_, "v");
    }

    void visit(const ast::NilPointerType&) final {
      assert(false);
    }

    void visit(const ast::ErrorType&) final {
      assert(false);
    }

    void visit(const ast::UnsizedIntegerType&) final {
      assert(false);
    }

    void visit(const ast::ArrayType& type) final {
      absl::StrAppend(&builder_, "A");
      mangle(type.element_type());
      absl::StrAppend(&builder_, type.size(), "_");
    }

    void visit(const ast::IndirectionType&) final {
      assert(false);
    }

  private:
    void mangle(const ast::Type& type) noexcept {
      auto start_index = builder_.size(); // not `- 1` because chars havent been added yet

      type.accept(this);

      if (type.is_one_of(ast::TypeType::user_defined, ast::TypeType::dyn_interface)) {
        auto substr = std::string_view{builder_}.substr(start_index);

        // if the mangled type is registered in the substitutions, replace it.
        // otherwise, we register it
        if (auto it = substitutions_.find(substr); it != substitutions_.end()) {
          builder_.resize(start_index);

          absl::StrAppend(&builder_, "Z", it->second, "_");
        } else {
          substitutions_.emplace(std::string{substr}, code_++);
        }
      }
    }

    void build_module_prefix(const ast::FullyQualifiedID& id) noexcept {
      auto parts = absl::StrSplit(id.module_string(), "::");

      for (auto part : parts) {
        if (part.empty()) {
          continue;
        }

        absl::StrAppend(&builder_, part.size(), part);
      }
    }

    std::string builder_;
    std::int64_t code_ = 0;
    absl::flat_hash_map<std::string, std::int64_t> substitutions_;
  };

  class MangleNode final : public ast::DeclarationVisitor<void> {
  public:
    void visit(ast::ImportDeclaration*) final {}

    void visit(ast::ImportFromDeclaration*) final {}

    void visit(ast::FnDeclaration* declaration) final {
      declaration->set_mangled(gal::mangle(*declaration));
    }

    void visit(ast::StructDeclaration*) final {}

    void visit(ast::ClassDeclaration*) final {}

    void visit(ast::TypeDeclaration*) final {}

    void visit(ast::MethodDeclaration*) final {}

    void visit(ast::ExternalFnDeclaration* declaration) final {
      declaration->set_mangled(gal::mangle(*declaration));
    }

    void visit(ast::ExternalDeclaration*) final {}

    void visit(ast::ConstantDeclaration* declaration) final {
      declaration->set_mangled(gal::mangle(*declaration));
    }
  };

  class Demangler final {
  public:
    explicit Demangler(std::string_view mangled) noexcept : mangled_{mangled}, pos_{2} {}

    std::string demangle() noexcept {
      static absl::flat_hash_map<std::string, std::string> demangle_exceptions{
          {"__gallium_user_main", "fn ::main() -> void"},
      };

      if (auto it = demangle_exceptions.find(mangled_); it != demangle_exceptions.end()) {
        return it->second;
      }

      if (mangled_.size() < 3 || mangled_.substr(0, 2) != "_G") {
        return std::string{mangled_};
      }

      absl::StrAppend(&builder_, "::");

      while (pos_ < mangled_.size()) {
        switch (mangled_[pos_]) {
          case 'F': return function(); break;
          case 'C': return constant(); break;
          default: module_part(); break;
        }
      }

      assert(false);

      return "";
    }

    void module_part() noexcept {
      while (std::isdigit(mangled_[pos_])) {
        part_with_len();
        absl::StrAppend(&builder_, "::");
      }
    }

    std::string function() noexcept {
      ++pos_;

      part_with_len();

      auto does_throw = (mangled_[pos_++] == 'T');

      absl::StrAppend(&builder_, "(");
      while (mangled_[pos_] != 'E') {
        type();

        if (mangled_[pos_] != 'E') {
          absl::StrAppend(&builder_, ", ");
        }
      }
      ++pos_; // eat the `E`
      absl::StrAppend(&builder_, ")", does_throw ? " throws -> " : " -> ");
      type();

      builder_.insert(0, "fn ");
      return std::move(builder_);
    }

    std::string constant() noexcept {
      ++pos_;

      part_with_len();

      absl::StrAppend(&builder_, ": ");
      type();

      builder_.insert(0, "const ");
      return std::move(builder_);
    }

    void type() noexcept {
      switch (mangled_[pos_++]) {
        case 'v': absl::StrAppend(&builder_, "void"); break;
        case 'a': absl::StrAppend(&builder_, "byte"); break;
        case 'b': absl::StrAppend(&builder_, "bool"); break;
        case 'c': absl::StrAppend(&builder_, "char"); break;
        case 'd': absl::StrAppend(&builder_, "u8"); break;
        case 'e': absl::StrAppend(&builder_, "u16"); break;
        case 'f': absl::StrAppend(&builder_, "u32"); break;
        case 'g': absl::StrAppend(&builder_, "u64"); break;
        case 'h': absl::StrAppend(&builder_, "u128"); break;
        case 'i': absl::StrAppend(&builder_, "usize"); break;
        case 'j': absl::StrAppend(&builder_, "i8"); break;
        case 'k': absl::StrAppend(&builder_, "i16"); break;
        case 'l': absl::StrAppend(&builder_, "i32"); break;
        case 'm': absl::StrAppend(&builder_, "i64"); break;
        case 'n': absl::StrAppend(&builder_, "i128"); break;
        case 'o': absl::StrAppend(&builder_, "isize"); break;
        case 'p': absl::StrAppend(&builder_, "f32"); break;
        case 'q': absl::StrAppend(&builder_, "f64"); break;
        case 'r': absl::StrAppend(&builder_, "f128"); break;
        case 'P':
          absl::StrAppend(&builder_, "*const ");
          type();
          break;
        case 'Q':
          absl::StrAppend(&builder_, "*mut ");
          type();
          break;
        case 'R':
          absl::StrAppend(&builder_, "&");
          type();
          break;
        case 'S':
          absl::StrAppend(&builder_, "&mut ");
          type();
          break;
        case 'A':
          absl::StrAppend(&builder_, "[");
          type();
          absl::StrAppend(&builder_, "; ", digits(), "]");
          ++pos_; // eat the _
          break;
        case 'B':
          absl::StrAppend(&builder_, "[");
          type();
          absl::StrAppend(&builder_, "]");
          break;
        case 'C':
          absl::StrAppend(&builder_, "[mut ");
          type();
          absl::StrAppend(&builder_, "]");
          break;
        case 'F': {
          absl::StrAppend(&builder_, "fn(");

          auto throws = (mangled_[pos_++] == 'T');

          while (mangled_[pos_] != 'E') {
            type();

            if (mangled_[pos_] != 'E') {
              absl::StrAppend(&builder_, ", ");
            }
          }

          ++pos_; // eat `E`

          absl::StrAppend(&builder_, ") ", throws ? "throws " : "", "-> ");
          type();
          break;
        }
        case 'Z':
          absl::StrAppend(&builder_, substitutions_.at(digits()));
          ++pos_; // eat `_`
          break;
        default: {
          --pos_;                       // hack to make it go back by a char for the len
          auto start = builder_.size(); // intentionally not -1, last char is currently ( not the first char of type
          absl::StrAppend(&builder_, "::");
          module_part();

          switch (mangled_[pos_++]) {
            case 'D': builder_.insert(start, "dyn "); [[fallthrough]];
            case 'U': part_with_len(); break;
            default: assert(false); break;
          }

          substitutions_.push_back(builder_.substr(start, builder_.size() - start));

          break;
        }
      }
    }

    void part_with_len() {
      auto len = digits();
      auto start = pos_;
      for (auto i = std::uint64_t{0}; i != len; ++i) {
        ++pos_;
      }

      absl::StrAppend(&builder_, mangled_.substr(start, pos_ - start));
    }

    std::uint64_t digits() noexcept {
      auto start = pos_;

      while (std::isdigit(mangled_[pos_])) {
        ++pos_;
      }

      return std::get<std::uint64_t>(gal::from_digits(mangled_.substr(start, pos_ - start)));
    }

  private:
    std::string_view mangled_;
    std::size_t pos_;
    std::string builder_;
    std::vector<std::string> substitutions_;
  };
} // namespace

std::string gal::mangle(const ast::Declaration& node) noexcept {
  return Mangler().mangle_decl(node);
}

std::string gal::demangle(std::string_view mangled) noexcept {
  // if it doesn't have `_G` at the beginning, it's not a mangled symbol
  return Demangler(mangled).demangle();
}

void gal::mangle_program(ast::Program* program) noexcept {
  for (auto& decl : program->decls_mut()) {
    auto mangler = MangleNode();

    decl->accept(&mangler);
  }
}