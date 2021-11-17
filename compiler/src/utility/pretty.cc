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
#include "absl/container/flat_hash_map.h"
#include "absl/strings/charconv.h"
#include "absl/strings/str_cat.h"
#include <charconv>

namespace ast = gal::ast;
namespace colors = gal::colors;

namespace {
  class ASTPrinter final : public ast::ConstDeclarationVisitor<void>,
                           public ast::ConstStatementVisitor<void>,
                           public ast::ConstExpressionVisitor<void>,
                           public ast::ConstTypeVisitor<std::string> {
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
      print_member("exported: ", lit_str(node.exported()));
      print_member("module: ", id_str(node.mod().to_string()));
      print_last_member("alias: ", node.alias().has_value() ? id_str(*node.alias()) : colors::yellow("n/a"));
    }

    void visit(const ast::ImportFromDeclaration& node) final {
      print_initial(colors::bold_red("import-from decl"));
      print_member("exported: ", lit_str(node.exported()));
      print_last_list("entities", node.imported_entities(), [this](auto& entity) {
        print_initial(entity.to_string());
      });
    }

    void visit(const ast::FnDeclaration& node) final {
      print_initial(colors::bold_red("fn decl"));
      print_member("exported: ", lit_str(node.exported()));
      print_member("external: ", lit_str(node.external()));
      print_proto(node.proto(), false);
      accept_last_member("body: ", node.body());
    }

    void visit(const ast::StructDeclaration& node) final {
      print_initial(colors::bold_red("struct decl"));
      print_last_list("members: ", node.fields(), [this](auto& field) {
        print_initial("field");
        print_member("name: ", field.name);
        accept_last_member("type: ", *field.type);
      });
    }

    void visit(const ast::ClassDeclaration& node) final {
      (void)node;
      print_initial("ast::ClassDeclaration");
    }

    void visit(const ast::TypeDeclaration& node) final {
      print_initial(colors::bold_red("type decl"));
      print_member("name: ", colors::green(node.name()));
      accept_last_member("type: ", node.aliased());
    }

    void visit(const ast::MethodDeclaration& node) final {
      (void)node;
      print_initial("ast::MethodDeclaration");
    }

    void visit(const ast::ExternalDeclaration& node) final {
      print_initial(decl_str("external"));
      print_last_list("functions: ", node.prototypes(), [this](auto& proto) {
        print_initial(colors::red("ffi fn"));
        print_proto(proto, true);
      });
    }

    void visit(const ast::BindingStatement& node) final {
      print_initial(stmt_str("binding"));
      print_member("name: ", id_str(node.name()));

      if (auto hint = node.hint()) {
        accept_member("type hint: ", **hint);
      } else {
        print_member("type hint: n/a");
      }

      accept_last_member("initializer: ", node.initializer());
    }

    void visit(const ast::ExpressionStatement& node) final {
      print_initial(stmt_str("expr"));
      accept_last_member("expr: ", node.expr());
    }

    void visit(const ast::AssertStatement& node) final {
      print_initial(stmt_str("assert"));
      accept_member("assertion: ", node.assertion());
      print_last_member("message: ", lit_str(node.message().text()));
    }

    void visit(const ast::StringLiteralExpression& node) final {
      auto formatter = [](auto* out, auto val) {
        *out += +val;
      };

      print_expr("string literal", node);
      print_member("as string: ", lit_str(node.text()));
      print_last_member("as bytes: [", absl::StrJoin(node.text_unquoted(), ", ", formatter), "]");
    }

    void visit(const ast::IntegerLiteralExpression& node) final {
      print_expr("integer literal", node);
      print_last_member("value: ", lit_str(node.value(), std::numeric_limits<std::uint64_t>::digits10));
    }

    void visit(const ast::FloatLiteralExpression& node) final {
      print_expr("float literal", node);
      print_last_member("value: ", lit_str(node.value(), node.str_len()));
    }

    void visit(const ast::BoolLiteralExpression& node) final {
      print_expr("bool literal", node);
      print_member("value: ", lit_str(node.value()));
    }

    void visit(const ast::CharLiteralExpression& node) final {
      print_expr("char literal", node);
      print_member("byte value: ", lit_str(node.value(), std::numeric_limits<std::uint8_t>::digits10));
      print_last_member("value as `char`: ", lit_str(static_cast<char>(node.value())));
    }

    void visit(const ast::NilLiteralExpression& node) final {
      print_expr<true>("nil literal", node);
    }

    void visit(const ast::UnqualifiedIdentifierExpression& node) final {
      print_expr("unqual-id", node);
      print_member("module prefix: ",
          node.id().prefix().has_value() && !node.id().prefix()->parts().empty()
              ? id_str(node.id().prefix()->to_string())
              : "n/a");
      print_last_member("id: ", id_str(node.id().name()));
    }

    void visit(const ast::IdentifierExpression& node) final {
      print_expr("id", node);
      print_member("module prefix: ", id_str(node.id().to_string()));
      print_last_member("id: ", id_str(node.id().name()));
    }

    void visit(const ast::CallExpression& node) final {
      print_expr("call", node);
      accept_member("callee: ", node.callee());
      print_last_list("args: ", node.args(), [this](auto& ptr) {
        print_initial("argument");
        accept_last_member("value: ", *ptr);
      });
    }

    void visit(const ast::MethodCallExpression& node) final {
      (void)node;
      print_initial("ast::MethodCallExpression");
    }

    void visit(const ast::StaticMethodCallExpression& node) final {
      (void)node;
      print_initial("ast::StaticMethodCallExpression");
    }

    void visit(const ast::IndexExpression& node) final {
      print_expr("index", node);
      accept_member("callee: ", node.callee());
      print_last_list("args: ", node.args(), [this](auto& ptr) {
        print_initial("index argument");
        accept_last_member("value: ", *ptr);
      });
    }

    void visit(const ast::FieldAccessExpression& node) final {
      print_expr("field access", node);
      accept_member("object: ", node.object());
      print_last_member("field name: ", node.field_name());
    }

    void visit(const ast::GroupExpression& node) final {
      print_expr("group", node);
      accept_last_member("expr: ", node.expr());
    }

    void visit(const ast::UnaryExpression& node) final {
      print_expr("unary", node);
      print_member("op: ", colors::red(gal::unary_op_string(node.op())));
      accept_last_member("expr: ", node.expr());
    }

    void visit(const ast::BinaryExpression& node) final {
      print_expr("binary", node);
      print_member("op: ", colors::red(gal::binary_op_string(node.op())));
      accept_member("lhs: ", node.lhs());
      accept_last_member("rhs: ", node.rhs());
    }

    void visit(const ast::CastExpression& node) final {
      print_expr("cast", node);
      print_member("unsafe: ", lit_str(node.unsafe()));
      accept_member("casting to: ", node.cast_to());
      accept_last_member("castee: ", node.castee());
    }

    void visit(const ast::IfThenExpression& node) final {
      print_expr("if-then", node);
      accept_member("condition: ", node.condition());
      accept_member("if-true: ", node.true_branch());
      accept_last_member("if-false: ", node.false_branch());
    }

    void visit(const ast::IfElseExpression& node) final {
      print_expr("if-else", node);
      accept_member("condition: ", node.condition());
      accept_member("body: ", node.block());

      if (auto blocks = node.elif_blocks(); !blocks.empty()) {
        print_list("elif-blocks: ", blocks, [this](auto& block) {
          print_initial(colors::yellow("elif-block"));
          accept_member("condition: ", *block.condition);
          accept_last_member("body: ", *block.block);
        });
      } else {
        print_member("elif-blocks: n/a");
      }

      if (auto block = node.else_block()) {
        accept_last_member("else-block: ", **block);
      } else {
        print_last_member("else-block: n/a");
      }
    }

    void visit(const ast::BlockExpression& node) final {
      print_expr("block", node);
      print_last_list("stmts: ", node.statements(), [this](auto& ptr) {
        accept_initial(*ptr);
      });
    }

    void visit(const ast::LoopExpression& node) final {
      print_expr("loop", node);
      accept_last_member("body: ", node.body());
    }

    void visit(const ast::WhileExpression& node) final {
      print_expr("while", node);
      accept_member("condition: ", node.condition());
      accept_last_member("body: ", node.body());
    }

    void visit(const ast::ForExpression& node) final {
      print_expr("for", node);
      print_member("loop var name: ", node.loop_variable());
      print_member("loop direction: ",
          node.loop_direction() == gal::ast::ForExpression::Direction::up_to ? "up-to" : "down-to");
      accept_member("loop initializer: ", node.init());
      accept_last_member("body: ", node.body());
    }

    void visit(const ast::ReturnExpression& node) final {
      print_expr("return", node);

      if (auto return_value = node.value()) {
        accept_last_member("return value: ", **return_value);
      } else {
        print_last_member("return value: n/a");
      }
    }

    void visit(const ast::BreakExpression& node) final {
      print_expr("break", node);

      if (auto return_value = node.value()) {
        accept_last_member("yield value: ", **return_value);
      } else {
        print_last_member("yield value: n/a");
      }
    }

    void visit(const ast::ContinueExpression& node) final {
      print_expr<true>("continue", node);
    }

    void visit(const ast::ReferenceType& node) final {
      auto rest = node.referenced().accept(this);

      if (node.mut()) {
        emplace(absl::StrCat(colors::magenta("&mut"), " (", rest, ")"));
      } else {
        emplace(absl::StrCat(colors::magenta("&"), " (", rest, ")"));
      }
    }

    void visit(const ast::SliceType& node) final {
      auto rest = node.sliced().accept(this);

      emplace(absl::StrCat(colors::red("["), "(", rest, ")", colors::red("]")));
    }

    void visit(const ast::PointerType& node) final {
      auto rest = node.pointed().accept(this);

      if (node.mut()) {
        emplace(absl::StrCat(colors::yellow("*mut"), " (", rest, ")"));
      } else {
        emplace(absl::StrCat(colors::yellow("*const"), " (", rest, ")"));
      }
    }

    void visit(const ast::BuiltinIntegralType& node) final {
      if (auto width = ast::width_of(node.width())) {
        emplace(absl::StrCat(colors::code_blue, node.has_sign() ? "i" : "u", *width, colors::code_reset));
      } else {
        emplace(absl::StrCat(colors::code_blue, node.has_sign() ? "isize" : "usize", colors::code_reset));
      }
    }

    void visit(const ast::BuiltinFloatType& node) final {
      using ast::FloatWidth;

      switch (node.width()) {
        case FloatWidth::ieee_single: emplace(colors::magenta("f32")); break;
        case FloatWidth::ieee_double: emplace(colors::magenta("f64")); break;
        case FloatWidth::ieee_quadruple: emplace(colors::magenta("f128")); break;
        default:
          assert(false);
          emplace("");
          break;
      }
    }

    void visit(const ast::BuiltinByteType&) final {
      emplace(colors::red("byte"));
    }

    void visit(const ast::BuiltinBoolType&) final {
      emplace(colors::red("bool"));
    }

    void visit(const ast::BuiltinCharType&) final {
      emplace(colors::red("char"));
    }

    void visit(const ast::UnqualifiedUserDefinedType& node) final {
      emplace(absl::StrCat(colors::code_green, "unqualified `", node.id().to_string(), "`", colors::code_reset));
    }

    void visit(const ast::UserDefinedType& node) final {
      emplace(absl::StrCat(colors::code_green, "`", node.id().to_string(), "`", colors::code_reset));
    }

    void visit(const ast::FnPointerType& node) final {
      auto args = absl::StrJoin(node.args(), ", ", [this](auto* out, auto& ptr) {
        absl::StrAppend(out, "(", ptr->accept(this), ")");
      });

      emplace(absl::StrCat(colors::red("fn"),
          "(",
          args,
          ") ",
          colors::red("-> "),
          "(",
          node.return_type().accept(this),
          ")"));
    }

    void visit(const ast::UnqualifiedDynInterfaceType& node) final {
      emplace(absl::StrCat(colors::code_green,
          "unqualified ",
          colors::magenta("dyn"),
          " `",
          node.id().to_string(),
          "`",
          colors::code_reset));
    }

    void visit(const ast::DynInterfaceType& node) final {
      emplace(absl::StrCat(colors::code_green,
          colors::magenta("dyn"),
          " `",
          node.id().to_string(),
          "`",
          colors::code_reset));
    }

    void visit(const ast::VoidType&) final {
      emplace(colors::bold_black("void"));
    }

  private:
    enum class PaddingMessage { spaces, bar_spaces };

    static std::string_view padding_message(PaddingMessage message_type) noexcept {
      switch (message_type) {
        case PaddingMessage::spaces: return "   ";
        case PaddingMessage::bar_spaces: return "│  ";
        default: return "   ";
      }
    }

    struct WriteToPadding {
    public:
      explicit WriteToPadding(ASTPrinter* printer, std::string_view message) noexcept
          : len_{message.size()},
            printer_{printer} {
        absl::StrAppend(&printer_->padding_, message);
      }

      ~WriteToPadding() {
        auto& padding = printer_->padding_;

        padding.resize(padding.size() - len_);
      }

    private:
      std::size_t len_;
      ASTPrinter* printer_;
    };

    void print_proto(const ast::FnPrototype& proto, bool last) noexcept {
      print_member("name: ", id_str(proto.name()));
      print_list("args: ", proto.args(), [this](auto& arg) {
        print_initial(colors::bold_yellow("arg"));
        print_member("name: ", id_str(arg.name));
        accept_last_member("type: ", *arg.type);
      });
      print_list("attributes: ", proto.attributes(), [this](auto& attribute) {
        print_initial(colors::cyan(attribute_to_str(attribute)));
      });

      if (last) {
        accept_last_member("return type: ", proto.return_type());
      } else {
        accept_member("return type: ", proto.return_type());
      }
    }

    static std::string id_str(std::string_view name) noexcept {
      return colors::green(name);
    }

    template <typename T> static std::string lit_str(T lit, std::size_t str_len = 0) noexcept {
      if constexpr (std::is_same_v<T, bool>) {
        return colors::blue(lit ? "true" : "false");
      } else if constexpr (std::is_same_v<T, char>) {
        return std::string{lit};
      } else if constexpr (std::is_same_v<T, std::string_view>) {
        return colors::blue(lit);
      } else {
        auto result = std::string(str_len, ' ');
        std::to_chars(result.data(), result.data() + result.size(), lit);
        return colors::blue(result);
      }
    }

    static std::string expr_str(std::string_view name) noexcept {
      return colors::bold_cyan(absl::StrCat(name, " expr"));
    }

    static std::string decl_str(std::string_view name) noexcept {
      return colors::bold_red(absl::StrCat(name, " decl"));
    }

    static std::string stmt_str(std::string_view name) noexcept {
      return colors::bold_yellow(absl::StrCat(name, " stmt"));
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
      absl::StrAppend(&final_, padding_, "├─ ", heading);
      auto _ = WriteToPadding(this, padding_message(PaddingMessage::bar_spaces));
      member.accept(this);
    }

    void accept_member(std::string_view heading, const ast::Type& type) noexcept {
      print_member(heading, "(", type.accept(this), ")");
    }

    template <typename T> void accept_last_member(std::string_view heading, const T& member) noexcept {
      absl::StrAppend(&final_, padding_, "└─ ", heading);
      auto _ = WriteToPadding(this, padding_message(PaddingMessage::spaces));
      member.accept(this);
    }

    void accept_last_member(std::string_view heading, const ast::Type& type) noexcept {
      print_last_member(heading, "(", type.accept(this), ")");
    }

    template <typename T, typename Fn> void print_list(std::string_view heading, absl::Span<T> data, Fn f) noexcept {
      absl::StrAppend(&final_, padding_, "├─ ", heading);
      auto _ = WriteToPadding(this, padding_message(PaddingMessage::bar_spaces));
      print_list_internal(data, std::move(f));
    }

    template <typename T, typename Fn>
    void print_last_list(std::string_view heading, absl::Span<T> data, Fn f) noexcept {
      absl::StrAppend(&final_, padding_, "└─ ", heading);
      auto _ = WriteToPadding(this, padding_message(PaddingMessage::spaces));
      print_list_internal(data, std::move(f));
    }

    template <typename T, typename Fn> void print_list_internal(absl::Span<T> data, Fn f) noexcept {
      if (data.empty()) {
        absl::StrAppend(&final_, "[ ]\n");
      } else {
        absl::StrAppend(&final_, "\n");
      }

      auto count = 0;
      for (auto it = data.begin(); it != data.end(); ++it, ++count) {
        auto message = std::string_view{};

        if ((it + 1) == data.end()) {
          absl::StrAppend(&final_, padding_, "└─ [", count, "]: ");
          message = padding_message(PaddingMessage::spaces);
        } else {
          absl::StrAppend(&final_, padding_, "├─ [", count, "]: ");
          message = padding_message(PaddingMessage::bar_spaces);
        }

        auto _ = WriteToPadding(this, message);

        f(*it);
      }
    }

    template <bool Last = false> void print_expr(std::string_view name, const ast::Expression& node) noexcept {
      print_initial(expr_str(name));

      if (auto type = node.result()) {
        if constexpr (Last) {
          accept_last_member("type of: ", **type);
        } else {
          accept_member("type of: ", **type);
        }
      } else {
        if constexpr (Last) {
          print_last_member("type of: n/a");
        } else {
          print_member("type of: n/a");
        }
      }
    }

    std::string padding_;
    std::string final_;
  };
} // namespace

std::string_view gal::unary_op_string(ast::UnaryOp op) noexcept {
  static absl::flat_hash_map<ast::UnaryOp, std::string_view> lookup{{ast::UnaryOp::dereference, "*"},
      {ast::UnaryOp::logical_not, "not"},
      {ast::UnaryOp::bitwise_not, "~"},
      {ast::UnaryOp::mut_ref_to, "&mut"},
      {ast::UnaryOp::ref_to, "&"},
      {ast::UnaryOp::negate, "-"}};

  // will crash if the value isn't in the map
  return lookup.at(op);
}

std::string_view gal::binary_op_string(ast::BinaryOp op) noexcept {
  static absl::flat_hash_map<ast::BinaryOp, std::string_view> lookup{{ast::BinaryOp::mul, "*"},
      {ast::BinaryOp::div, "/"},
      {ast::BinaryOp::mod, "%"},
      {ast::BinaryOp::add, "+"},
      {ast::BinaryOp::sub, "-"},
      {ast::BinaryOp::lt, "<"},
      {ast::BinaryOp::gt, ">"},
      {ast::BinaryOp::lt_eq, "<="},
      {ast::BinaryOp::gt_eq, ">="},
      {ast::BinaryOp::equals, "=="},
      {ast::BinaryOp::not_equal, "!="},
      {ast::BinaryOp::left_shift, "<<"},
      {ast::BinaryOp::right_shift, ">>"},
      {ast::BinaryOp::bitwise_and, "&"},
      {ast::BinaryOp::bitwise_or, "|"},
      {ast::BinaryOp::bitwise_xor, "^"},
      {ast::BinaryOp::logical_and, "and"},
      {ast::BinaryOp::logical_or, "or"},
      {ast::BinaryOp::logical_xor, "xor"},
      {ast::BinaryOp::assignment, ":="},
      {ast::BinaryOp::add_eq, "+="},
      {ast::BinaryOp::sub_eq, "-="},
      {ast::BinaryOp::mul_eq, "*="},
      {ast::BinaryOp::div_eq, "/="},
      {ast::BinaryOp::mod_eq, "%="},
      {ast::BinaryOp::left_shift_eq, "<<="},
      {ast::BinaryOp::right_shift_eq, ">>="},
      {ast::BinaryOp::bitwise_and_eq, "&="},
      {ast::BinaryOp::bitwise_or_eq, "|="},
      {ast::BinaryOp::bitwise_xor_eq, "^="}};

  // will hard-crash if the value isn't in the map
  return lookup.at(op);
}

std::string gal::pretty_print(const ast::Program& program) noexcept {
  return ASTPrinter().print(program);
}