//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./pretty.h"
#include "../ast/visitors.h"
#include "absl/strings/str_cat.h"

namespace ast = gal::ast;
namespace colors = gal::colors;

namespace {
  class ASTPrinter final : public ast::ConstDeclarationVisitor<void>,
                           public ast::ConstStatementVisitor<void>,
                           public ast::ConstExpressionVisitor<void>,
                           public ast::ConstTypeVisitor<void> {
  public:
    std::string print(const ast::Program& program) noexcept {
      padding_ = "";
      final_ = "";

      print_initial("program");
      print_last_list("declarations: ", program.decls(), [this](auto& ptr) {
        accept_initial(*ptr);
      });

      return std::move(final_);
    }

    void visit(const ast::ImportDeclaration& node) final {
      print_initial(colors::bold_red("import decl"));
      print_member("exported: ", bool_to_str(node.exported()));
      print_member("module: ", colors::green(node.mod().to_string()));
      print_last_member("alias: ", node.alias().has_value() ? colors::blue(*node.alias()) : colors::yellow("n/a"));
    }

    void visit(const ast::ImportFromDeclaration& node) final {
      print_initial(colors::bold_red("import-from decl"));
      print_member("exported: ", bool_to_str(node.exported()));
      print_last_list("entities", node.imported_entities(), [this](auto& entity) {
        print_initial(entity.to_string());
      });
    }

    void visit(const ast::FnDeclaration& node) final {
      print_initial(colors::bold_red("fn decl"));
      print_member("exported: ", bool_to_str(node.exported()));
      print_member("external: ", bool_to_str(node.external()));
      print_proto(node.proto());
      accept_last_member("body: ", node.body());
    }

    void visit(const ast::StructDeclaration& node) final {
      (void)node;
    }

    void visit(const ast::ClassDeclaration& node) final {
      (void)node;
    }

    void visit(const ast::TypeDeclaration& node) final {
      print_initial(colors::bold_red("type decl"));
      print_member("name: ", colors::green(node.name()));
      accept_last_member("type: ", node.aliased());
    }

    void visit(const ast::MethodDeclaration& node) final {
      (void)node;
    }

    void visit(const ast::ExternalDeclaration& node) final {
      print_initial(colors::bold_red("external decl"));
      print_last_list("functions: ", node.prototypes(), [this](auto& proto) {
        print_initial(colors::red("extern fn"));
        print_proto(proto);
      });
    }

    void visit(const ast::BindingStatement& node) final {
      (void)node;
    }

    void visit(const ast::ExpressionStatement& node) final {
      (void)node;
    }

    void visit(const ast::AssertStatement& node) final {
      (void)node;
    }

    void visit(const ast::StringLiteralExpression& node) final {
      (void)node;
    }

    void visit(const ast::IntegerLiteralExpression& node) final {
      (void)node;
    }

    void visit(const ast::FloatLiteralExpression& node) final {
      (void)node;
    }

    void visit(const ast::BoolLiteralExpression& node) final {
      (void)node;
    }

    void visit(const ast::CharLiteralExpression& node) final {
      (void)node;
    }

    void visit(const ast::NilLiteralExpression& node) final {
      (void)node;
    }

    void visit(const ast::UnqualifiedIdentifierExpression& node) final {
      (void)node;
    }

    void visit(const ast::IdentifierExpression& node) final {
      (void)node;
    }

    void visit(const ast::CallExpression& node) final {
      (void)node;
    }

    void visit(const ast::MethodCallExpression& node) final {
      (void)node;
    }

    void visit(const ast::StaticMethodCallExpression& node) final {
      (void)node;
    }

    void visit(const ast::IndexExpression& node) final {
      (void)node;
    }

    void visit(const ast::FieldAccessExpression& node) final {
      (void)node;
    }

    void visit(const ast::GroupExpression& node) final {
      (void)node;
    }

    void visit(const ast::UnaryExpression& node) final {
      (void)node;
    }

    void visit(const ast::BinaryExpression& node) final {
      (void)node;
    }

    void visit(const ast::CastExpression& node) final {
      (void)node;
    }

    void visit(const ast::IfThenExpression& node) final {
      (void)node;
    }

    void visit(const ast::IfElseExpression& node) final {
      (void)node;
    }

    void visit(const ast::BlockExpression& node) final {
      (void)node;
    }

    void visit(const ast::LoopExpression& node) final {
      (void)node;
    }

    void visit(const ast::WhileExpression& node) final {
      (void)node;
    }

    void visit(const ast::ForExpression& node) final {
      (void)node;
    }

    void visit(const ast::ReturnExpression& node) final {
      (void)node;
    }

    void visit(const ast::BreakExpression& node) final {
      (void)node;
    }

    void visit(const ast::ContinueExpression& node) final {
      (void)node;
    }

    void visit(const ast::ReferenceType& node) final {
      (void)node;
    }

    void visit(const ast::SliceType& node) final {
      (void)node;
    }

    void visit(const ast::PointerType& node) final {
      (void)node;
    }

    void visit(const ast::BuiltinIntegralType& node) final {
      (void)node;
    }

    void visit(const ast::BuiltinFloatType& node) final {
      (void)node;
    }

    void visit(const ast::BuiltinByteType& node) final {
      (void)node;
    }

    void visit(const ast::BuiltinBoolType& node) final {
      (void)node;
    }

    void visit(const ast::BuiltinCharType& node) final {
      (void)node;
    }

    void visit(const ast::UnqualifiedUserDefinedType& node) final {
      (void)node;
    }

    void visit(const ast::UserDefinedType& node) final {
      (void)node;
    }

    void visit(const ast::FnPointerType& node) final {
      (void)node;
    }

    void visit(const ast::UnqualifiedDynInterfaceType& node) final {
      (void)node;
    }

    void visit(const ast::DynInterfaceType& node) final {
      (void)node;
    }

    void visit(const ast::VoidType& node) final {
      (void)node;
    }

  private:
    void print_proto(const ast::FnPrototype& proto) noexcept {
      print_member("name: ", colors::green(proto.name()));
      print_list("args", proto.args(), [this](auto& arg) {
        print_initial(colors::bold_yellow("arg"));
        print_member("name: ", colors::green(arg.name));
        accept_last_member("type: ", *arg.type);
      });
      print_list("attributes", proto.attributes(), [this](auto& attribute) {
        print_initial(colors::cyan(attribute_to_str(attribute)));
      });
      accept_member("return type: ", proto.return_type());
    }

    static std::string bool_to_str(bool value) noexcept {
      return colors::blue(value ? "true" : "false");
    }

    static std::string attribute_to_str(const ast::Attribute& attribute) noexcept {
      using Type = ast::AttributeType;

      switch (attribute.type) {
        case Type::builtin_always_inline: return "always-inline";
        case Type::builtin_arch: return absl::StrCat("arch (", attribute.args[0], ")");
        case Type::builtin_cold: return "cold";
        case Type::builtin_hot: return "hot";
        case Type::builtin_inline: return "inline";
        case Type::builtin_malloc: return "malloc";
        case Type::builtin_no_inline: return "no-inline";
        case Type::builtin_noreturn: return "no-return";
        case Type::builtin_pure: return "pure";
        case Type::builtin_throws: return "throws";
        default: assert(false); return "";
      }
    }

    template <typename... Args> void print_initial(Args&&... args) noexcept {
      absl::StrAppend(&final_, args..., "\n");
    }

    template <typename... Args> void print(Args&&... args) noexcept {
      absl::StrAppend(&final_, padding_, args..., "\n");
    }

    template <typename... Args> void print_member(Args&&... args) noexcept {
      print("├─ ", args...);
    }

    template <typename... Args> void print_last_member(Args&&... args) noexcept {
      print("└─ ", args...);
    }

    template <typename T> void accept_initial(const T& member) noexcept {
      member.accept(this);
    }

    template <typename T> void accept_member(std::string_view heading, const T& member) noexcept {
      print_member(heading);
      padding_ += "│  ";
      member.accept(this);
      padding_ = padding_.substr(0, padding_.size() - 3);
    }

    template <typename T> void accept_last_member(std::string_view heading, const T& member) noexcept {
      print_last_member(heading);
      padding_ += "   ";
      member.accept(this);
      padding_ = padding_.substr(0, padding_.size() - 3);
    }

    template <typename T, typename Fn> void print_list(std::string_view heading, absl::Span<T> data, Fn f) noexcept {
      print_member(heading);
      padding_ += "│  ";
      print_list_internal<T, Fn, false>(data, std::move(f));
      padding_ = padding_.substr(0, padding_.size() - 3);
    }

    template <typename T, typename Fn>
    void print_last_list(std::string_view heading, absl::Span<T> data, Fn f) noexcept {
      print_last_member(heading);
      padding_ += "   ";
      print_list_internal<T, Fn, true>(data, std::move(f));
      padding_ = padding_.substr(0, padding_.size() - 3);
    }

    template <typename T, typename Fn, bool Last> void print_list_internal(absl::Span<T> data, Fn f) noexcept {
      auto count = 0;

      for (auto it = data.begin(); it != data.end(); ++it, ++count) {
        if ((it + 1) == data.end()) {
          absl::StrAppend(&final_, padding_, "└─ [", count, "]: ");
        } else {
          absl::StrAppend(&final_, padding_, "├─ [", count, "]: ");
        }

        f(*it);
      }
    }

    std::string padding_;
    std::string final_;
  };
} // namespace

std::string gal::pretty_print(const ast::Program& program) noexcept {
  return ASTPrinter().print(program);
}