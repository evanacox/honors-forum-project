//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./predefined.h"
#include "../ast/program.h"
#include "absl/strings/match.h"
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

namespace ast = gal::ast;

namespace {
  std::unique_ptr<ast::Type> uint_type(int width) noexcept {
    return std::make_unique<ast::BuiltinIntegralType>(ast::SourceLoc::nonexistent(),
        false,
        static_cast<ast::IntegerWidth>(width));
  }

  std::unique_ptr<ast::Type> int_type(int width) noexcept {
    return std::make_unique<ast::BuiltinIntegralType>(ast::SourceLoc::nonexistent(),
        true,
        static_cast<ast::IntegerWidth>(width));
  }

  std::unique_ptr<ast::Type> int_native() noexcept {
    return std::make_unique<ast::BuiltinIntegralType>(ast::SourceLoc::nonexistent(),
        true,
        ast::IntegerWidth::native_width);
  }

  std::unique_ptr<ast::Type> uint_native() noexcept {
    return std::make_unique<ast::BuiltinIntegralType>(ast::SourceLoc::nonexistent(),
        false,
        ast::IntegerWidth::native_width);
  }

  std::unique_ptr<ast::Type> float_type(ast::FloatWidth width) noexcept {
    return std::make_unique<ast::BuiltinFloatType>(ast::SourceLoc::nonexistent(), width);
  }

  std::unique_ptr<ast::Type> slice_of(std::unique_ptr<ast::Type> type, bool mut) noexcept {
    return std::make_unique<ast::SliceType>(ast::SourceLoc::nonexistent(), mut, std::move(type));
  }

  std::unique_ptr<ast::Type> ptr_to(std::unique_ptr<ast::Type> type, bool mut) noexcept {
    return std::make_unique<ast::PointerType>(ast::SourceLoc::nonexistent(), mut, std::move(type));
  }

  std::unique_ptr<ast::Type> void_type() noexcept {
    return std::make_unique<ast::VoidType>(ast::SourceLoc::nonexistent());
  }

  std::unique_ptr<ast::Type> byte_type() noexcept {
    return std::make_unique<ast::BuiltinByteType>(ast::SourceLoc::nonexistent());
  }

  std::unique_ptr<ast::Type> bool_type() noexcept {
    return std::make_unique<ast::BuiltinBoolType>(ast::SourceLoc::nonexistent());
  }

  std::unique_ptr<ast::Type> char_type() noexcept {
    return std::make_unique<ast::BuiltinCharType>(ast::SourceLoc::nonexistent());
  }

  std::unique_ptr<ast::Declaration> create_builtin(std::string_view name,
      std::optional<std::vector<ast::Argument>> args = std::nullopt,
      std::optional<std::vector<ast::Attribute>> attributes = std::nullopt,
      std::optional<std::unique_ptr<ast::Type>> ret_type = std::nullopt) noexcept {
    auto proto = ast::FnPrototype(std::string{name},
        std::nullopt,
        std::move(args).value_or(std::vector<ast::Argument>{}),
        std::move(attributes).value_or(std::vector<ast::Attribute>{}),
        std::move(ret_type).value_or(void_type()));

    auto decl = std::make_unique<ast::ExternalFnDeclaration>(ast::SourceLoc::nonexistent(), false, std::move(proto));

    decl->set_injected();

    return decl;
  }

  void register_builtins(ast::Program* program) noexcept {
    auto loc = ast::SourceLoc::nonexistent();

    auto builtin_trap = create_builtin("__builtin_trap",
        std::nullopt,
        std::vector{ast::Attribute{ast::AttributeType::builtin_noreturn, {}}});

    auto builtin_string_ptr = create_builtin("__builtin_string_ptr",
        std::vector{ast::Argument(loc, "__1", slice_of(char_type(), false))},
        std::nullopt,
        std::make_unique<ast::PointerType>(ast::SourceLoc::nonexistent(), false, char_type()));

    auto builtin_string_len = create_builtin("__builtin_string_len",
        std::vector{ast::Argument(loc, "__1", slice_of(char_type(), false))},
        std::nullopt,
        std::make_unique<ast::BuiltinIntegralType>(loc, false, ast::IntegerWidth::native_width));

    auto builtin_black_box =
        create_builtin("__builtin_black_box", std::vector{ast::Argument(loc, "__1", ptr_to(byte_type(), false))});

    auto externals = gal::into_list(std::move(builtin_trap),
        std::move(builtin_string_ptr),
        std::move(builtin_string_len),
        std::move(builtin_black_box));

    auto node = std::make_unique<ast::ExternalDeclaration>(ast::SourceLoc::nonexistent(), false, std::move(externals));
    node->set_injected();
    program->add_decl(std::move(node));
  }

  std::unique_ptr<ast::Expression> char_literal(std::uint8_t value) noexcept {
    return std::make_unique<ast::CharLiteralExpression>(ast::SourceLoc::nonexistent(), value);
  }

  std::vector<std::unique_ptr<ast::Type>> generic_params() noexcept {
    return {};
  }

  std::unique_ptr<ast::Expression> create_id(std::string_view name) noexcept {
    return std::make_unique<ast::UnqualifiedIdentifierExpression>(ast::SourceLoc::nonexistent(),
        ast::UnqualifiedID(std::nullopt, std::string{name}),
        generic_params(),
        std::nullopt);
  }

  std::unique_ptr<ast::Expression> create_call(std::string_view name,
      std::unique_ptr<ast::Expression> arg1,
      std::unique_ptr<ast::Expression> arg2 = nullptr) {
    auto callee = create_id(name);
    auto args = gal::into_list(std::move(arg1));

    if (arg2 != nullptr) {
      args.push_back(std::move(arg2));
    }

    return std::make_unique<ast::CallExpression>(ast::SourceLoc::nonexistent(),
        std::move(callee),
        std::move(args),
        generic_params());
  }

  std::unique_ptr<ast::Expression> create_call(std::string_view name,
      std::string_view arg1,
      std::string_view arg2 = "") noexcept {
    return create_call(name, create_id(arg1), arg2.empty() ? nullptr : create_id(arg2));
  }

  std::unique_ptr<ast::Expression> create_cast(std::unique_ptr<ast::Expression> expr,
      std::unique_ptr<ast::Type> to) noexcept {
    return std::make_unique<ast::CastExpression>(ast::SourceLoc::nonexistent(), false, std::move(expr), std::move(to));
  }

  std::unique_ptr<ast::Expression> create_cast(std::string_view id, std::unique_ptr<ast::Type> to) noexcept {
    return create_cast(create_id(id), std::move(to));
  }

  std::unique_ptr<ast::BlockExpression> expr_into_block(std::vector<std::unique_ptr<ast::Expression>> exprs) noexcept {
    // need CTAD to deduce `Statement` not `ExpressionStatement`
    std::vector<std::unique_ptr<ast::Statement>> vec;
    vec.reserve(exprs.size());

    std::transform(exprs.begin(), exprs.end(), std::back_inserter(vec), [](std::unique_ptr<ast::Expression>& expr) {
      return std::make_unique<ast::ExpressionStatement>(ast::SourceLoc::nonexistent(), std::move(expr));
    });

    return std::make_unique<ast::BlockExpression>(ast::SourceLoc::nonexistent(), std::move(vec));
  }

  std::unique_ptr<ast::Declaration> create_stdlib_builtin(std::string_view name,
      std::vector<std::unique_ptr<ast::Type>> arg_types,
      std::unique_ptr<ast::BlockExpression> body) noexcept {
    std::vector<ast::Argument> args;
    args.reserve(arg_types.size());

    for (auto i = std::size_t{0}; i < arg_types.size(); ++i) {
      args.emplace_back(ast::SourceLoc::nonexistent(), absl::StrCat("__", i + 1), std::move(arg_types[i]));
    }

    auto proto = ast::FnPrototype(std::string{name},
        std::nullopt,
        std::move(args),
        std::vector{ast::Attribute{ast::AttributeType::builtin_stdlib, std::vector<std::string>{}}},
        void_type());

    auto fn = std::make_unique<ast::FnDeclaration>(ast::SourceLoc::nonexistent(),
        false,
        false,
        std::move(proto),
        std::move(body));

    fn->set_injected();

    return fn;
  }

  std::unique_ptr<ast::Declaration> create_print(std::vector<std::unique_ptr<ast::Type>> arg_types,
      std::unique_ptr<ast::BlockExpression> body) noexcept {
    return create_stdlib_builtin("print", std::move(arg_types), std::move(body));
  }

  std::unique_ptr<ast::Declaration> create_println(std::unique_ptr<ast::Type> arg_type,
      std::unique_ptr<ast::Type> arg2_type = nullptr) noexcept {
    auto call = create_call("print", "__1", arg2_type != nullptr ? "__2" : "");
    auto call2 = create_call("print", char_literal('\n'));

    return create_stdlib_builtin("println",
        arg2_type != nullptr ? gal::into_list(std::move(arg_type), std::move(arg2_type))
                             : gal::into_list(std::move(arg_type)),
        expr_into_block(gal::into_list(std::move(call), std::move(call2))));
  }

  std::unique_ptr<ast::Declaration> create_runtime_fn(std::string_view name,
      std::vector<std::unique_ptr<ast::Type>> args) {
    std::vector<ast::Argument> arguments;
    args.reserve(args.size());

    for (auto i = std::size_t{0}; i < args.size(); ++i) {
      arguments.emplace_back(ast::SourceLoc::nonexistent(), absl::StrCat("__", i + 1), std::move(args[i]));
    }

    return create_builtin(name, std::move(arguments));
  }

  void register_io_ffi(ast::Program* program) noexcept {
    //    fn __gallium_print_f32(x: f32, precision: i32) -> void
    //    fn __gallium_print_f64(x: f64, precision: i32) -> void
    //    fn __gallium_print_int(x: isize) -> void
    //    fn __gallium_print_uint(x: usize) -> void
    //    fn __gallium_print_char(s: char) -> void
    //    fn __gallium_print_string(s: *const char, n: usize) -> void

    auto print_f32 = create_runtime_fn("__gallium_print_f32",
        gal::into_list(float_type(ast::FloatWidth::ieee_single), int_type(32)));
    auto print_f64 = create_runtime_fn("__gallium_print_f64",
        gal::into_list(float_type(ast::FloatWidth::ieee_double), int_type(32)));
    auto print_isize = create_runtime_fn("__gallium_print_int", gal::into_list(int_native()));
    auto print_usize = create_runtime_fn("__gallium_print_uint", gal::into_list(uint_native()));
    auto print_char = create_runtime_fn("__gallium_print_char", gal::into_list(char_type()));
    auto print_str =
        create_runtime_fn("__gallium_print_string", gal::into_list(ptr_to(char_type(), false), uint_native()));

    auto external = std::make_unique<ast::ExternalDeclaration>(ast::SourceLoc::nonexistent(),
        false,
        gal::into_list(std::move(print_f32),
            std::move(print_f64),
            std::move(print_isize),
            std::move(print_usize),
            std::move(print_char),
            std::move(print_str)));

    external->set_injected();

    program->add_decl(std::move(external));
  }

  void register_io(ast::Program* program) noexcept {
    {
      // print(__1: [char]) -> void
      auto ptr = create_call("__builtin_string_ptr", "__1");
      auto len = create_call("__builtin_string_len", "__1");
      std::unique_ptr<ast::Expression> call = std::make_unique<ast::CallExpression>(ast::SourceLoc::nonexistent(),
          create_id("__gallium_print_string"),
          gal::into_list(std::move(ptr), std::move(len)),
          generic_params());

      program->add_decl(
          create_print(gal::into_list(slice_of(char_type(), false)), expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: char) -> void
      auto call = create_call("__gallium_print_char", "__1");

      program->add_decl(create_print(gal::into_list(char_type()), expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: f32, __2: i32) -> void
      auto call = create_call("__gallium_print_f32", "__1", "__2");

      program->add_decl(create_print(gal::into_list(float_type(ast::FloatWidth::ieee_single), int_type(32)),
          expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: f32) -> void
      auto precision = std::make_unique<ast::IntegerLiteralExpression>(ast::SourceLoc::nonexistent(), 5);
      auto call = create_call("__gallium_print_f32", create_id("__1"), create_cast(std::move(precision), int_type(32)));

      program->add_decl(create_print(gal::into_list(float_type(ast::FloatWidth::ieee_single)),
          expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: f64, __2: i32) -> void
      auto call = create_call("__gallium_print_f64", "__1", "__2");

      program->add_decl(create_print(gal::into_list(float_type(ast::FloatWidth::ieee_double), int_type(32)),
          expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: f64) -> void
      auto precision = std::make_unique<ast::IntegerLiteralExpression>(ast::SourceLoc::nonexistent(), 5);
      auto call = create_call("__gallium_print_f64", create_id("__1"), create_cast(std::move(precision), int_type(32)));

      program->add_decl(create_print(gal::into_list(float_type(ast::FloatWidth::ieee_double)),
          expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: i32) -> void
      auto call = create_call("__gallium_print_int", create_cast("__1", int_native()));

      program->add_decl(create_print(gal::into_list(int_type(32)), expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: i64) -> void
      auto call = create_call("__gallium_print_int", create_cast("__1", int_native()));

      program->add_decl(create_print(gal::into_list(int_type(64)), expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: isize) -> void
      auto call = create_call("__gallium_print_int", "__1");

      program->add_decl(create_print(gal::into_list(int_native()), expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: u32) -> void
      auto call = create_call("__gallium_print_uint", create_cast("__1", uint_native()));

      program->add_decl(create_print(gal::into_list(uint_type(32)), expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: u64) -> void
      auto call = create_call("__gallium_print_uint", create_cast("__1", uint_native()));

      program->add_decl(create_print(gal::into_list(uint_type(64)), expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: usize) -> void
      auto call = create_call("__gallium_print_uint", "__1");

      program->add_decl(create_print(gal::into_list(uint_native()), expr_into_block(gal::into_list(std::move(call)))));
    }

    {
      // print(__1: bool) -> void
      auto cond = create_id("__1");
      auto true_ish = std::make_unique<ast::StringLiteralExpression>(ast::SourceLoc::nonexistent(), "true");
      auto false_ish = std::make_unique<ast::StringLiteralExpression>(ast::SourceLoc::nonexistent(), "false");
      auto which = std::make_unique<ast::IfThenExpression>(ast::SourceLoc::nonexistent(),
          std::move(cond),
          std::move(true_ish),
          std::move(false_ish));

      auto call = create_call("print", std::move(which));

      program->add_decl(create_print(gal::into_list(bool_type()), expr_into_block(gal::into_list(std::move(call)))));
    }

    // println(__1: [char]) -> void
    program->add_decl(create_println(slice_of(char_type(), false)));

    // println(__1: char) -> void
    program->add_decl(create_println(char_type()));

    // println(__1: f32, __2: i32) -> void
    program->add_decl(create_println(float_type(ast::FloatWidth::ieee_single), int_type(32)));

    // println(__1: f32) -> void
    program->add_decl(create_println(float_type(ast::FloatWidth::ieee_single)));

    // println(__1: f64, __2: i32) -> void
    program->add_decl(create_println(float_type(ast::FloatWidth::ieee_double), int_type(32)));

    // println(__1: f64) -> void
    program->add_decl(create_println(float_type(ast::FloatWidth::ieee_double)));

    // println(__1: i32) -> void
    program->add_decl(create_println(int_type(32)));

    // println(__1: i64) -> void
    program->add_decl(create_println(int_type(64)));

    // println(__1: isize) -> void
    program->add_decl(create_println(int_native()));

    // println(__1: u32) -> void
    program->add_decl(create_println(uint_type(32)));

    // println(__1: u64) -> void
    program->add_decl(create_println(uint_type(64)));

    // println(__1: usize) -> void
    program->add_decl(create_println(uint_native()));

    // println(__1: bool) -> void
    program->add_decl(create_println(bool_type()));
  }
} // namespace

void gal::register_predefined(ast::Program* program) noexcept {
  register_builtins(program);
  register_io_ffi(program);
  register_io(program);
}

std::optional<std::unique_ptr<ast::Type>> gal::check_builtin(std::string_view name,
    absl::Span<const std::unique_ptr<ast::Expression>>) noexcept {
  if (!absl::StartsWith(name, "__builtin")) {
    return std::nullopt;
  }

  // TODO
  return {};
}
