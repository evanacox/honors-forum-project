//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./parser.h"
#include "../ast/nodes.h"
#include "../ast/program.h"
#include "../errors/reporter.h"
#include "../utility/misc.h"
#include "./parse_errors.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/charconv.h"
#include "absl/strings/str_split.h"
#include "antlr4-runtime.h"
#include "generated/GalliumBaseVisitor.h"
#include "generated/GalliumLexer.h"
#include "generated/GalliumParser.h"
#include <charconv>
#include <limits>
#include <optional>
#include <sstream>
#include <string_view>
#include <type_traits>

/*
 * the ANTLR C++ API is absolutely terrible and doesn't actually allow
 * `unique_ptr<Base>` when returning a `unique_ptr<Derived>` in visitors,
 * thus instead of a few member variables are used to hold
 * return values.
 *
 * as I have better things to do than rewrite the ANTLR C++ runtime for one
 * project, this will have to do.
 */

// convenience macro for using the `ASTGenerator::ret` overloading trick
// that returns an "antlrcpp::Any" while actually putting a return value
// in one of the member variables based on the type
#define RETURN(ptr) return ret(ptr)

namespace {
  namespace ast = gal::ast;

  class ASTGenerator final : public GalliumBaseVisitor {
  public:
    explicit ASTGenerator(gal::DiagnosticReporter* reporter) noexcept : diagnostics_{reporter} {}

    std::optional<ast::Program> into_ast(std::string_view source,
        std::filesystem::path path,
        GalliumParser::ParseContext* parse_tree) noexcept {
      auto decls = std::vector<std::unique_ptr<ast::Declaration>>{};
      path_ = std::move(path);
      original_ = source;

      for (auto* decl : parse_tree->modularizedDeclaration()) {
        visitModularizedDeclaration(decl);

        decls.push_back(return_value<ast::Declaration>());
      }

      if (diagnostics_->had_error()) {
        return std::nullopt;
      } else {
        return ast::Program(std::move(decls));
      }
    }

    antlrcpp::Any visitModularIdentifier(GalliumParser::ModularIdentifierContext* ctx) final {
      std::vector<std::string> module_parts;

      for (auto* node : ctx->IDENTIFIER()) {
        module_parts.push_back(node->toString());
      }

      return ast::ModuleID(ctx->isRoot != nullptr, std::move(module_parts));
    }

    antlrcpp::Any visitModularizedDeclaration(GalliumParser::ModularizedDeclarationContext* ctx) final {
      // either an export or an import decl, both are handled
      if (auto* decl = ctx->importDeclaration()) {
        exported_ = false;

        return visitImportDeclaration(decl);
      }

      return visitExportDeclaration(ctx->exportDeclaration());
    }

    antlrcpp::Any visitImportDeclaration(GalliumParser::ImportDeclarationContext* ctx) final {
      auto loc = loc_from(ctx);
      auto module_id = std::move(visitModularIdentifier(ctx->modularIdentifier()).as<ast::ModuleID>());

      if (auto* list = ctx->importList()) {
        std::vector<ast::FullyQualifiedID> ids;

        for (auto* id : list->identifierList()->IDENTIFIER()) {
          ids.emplace_back(module_id.to_string(), id->toString());
        }

        RETURN(std::make_unique<ast::ImportFromDeclaration>(loc_from(ctx), exported_, std::move(ids)));
      }

      auto alias = (ctx->alias != nullptr) ? std::make_optional(ctx->alias->toString())
                                           : std::optional<std::string>(std::nullopt);

      RETURN(
          std::make_unique<ast::ImportDeclaration>(loc_from(ctx), exported_, std::move(module_id), std::move(alias)));
    }

    antlrcpp::Any visitExportDeclaration(GalliumParser::ExportDeclarationContext* ctx) final {
      exported_ = ctx->exportKeyword != nullptr;

      return visitDeclaration(ctx->declaration());
    }

    antlrcpp::Any visitDeclaration(GalliumParser::DeclarationContext* ctx) final {
      // will handle actually going to the right one
      return visitChildren(ctx);
    }

    antlrcpp::Any visitConstDeclaration(GalliumParser::ConstDeclarationContext* ctx) final {
      auto name = ctx->IDENTIFIER()->getText();
      auto hint = parse_type(ctx->type());
      visitConstantExpr(ctx->constantExpr());
      auto init = return_value<ast::Expression>();

      RETURN(std::make_unique<ast::ConstantDeclaration>(loc_from(ctx),
          exported_,
          std::move(name),
          std::move(hint),
          std::move(init)));
    }

    antlrcpp::Any visitExternalDeclaration(GalliumParser::ExternalDeclarationContext* ctx) final {
      auto prototypes = std::vector<std::unique_ptr<ast::Declaration>>{};

      for (auto* fn : ctx->fnPrototype()) {
        auto proto = std::move(visitFnPrototype(fn).as<ast::FnPrototype>());

        prototypes.push_back(std::make_unique<ast::ExternalFnDeclaration>(loc_from(fn), exported_, std::move(proto)));
      }

      RETURN(std::make_unique<ast::ExternalDeclaration>(loc_from(ctx), exported_, std::move(prototypes)));
    }

    antlrcpp::Any visitFnPrototype(GalliumParser::FnPrototypeContext* ctx) final {
      std::vector<ast::Argument> args;
      if (auto* list = ctx->fnArgumentList()) {
        args = std::move(visitFnArgumentList(list).as<decltype(args)>());
      }

      std::vector<ast::Attribute> attributes;
      if (auto* list = ctx->fnAttributeList()) {
        attributes = std::move(visitFnAttributeList(list).as<decltype(attributes)>());
      }

      auto name = ctx->IDENTIFIER()->toString();

      visitType(ctx->type());
      auto return_type = return_value<ast::Type>();

      return ast::FnPrototype(std::move(name),
          std::nullopt,
          std::move(args),
          std::move(attributes),
          std::move(return_type));
    }

    antlrcpp::Any visitFnDeclaration(GalliumParser::FnDeclarationContext* ctx) final {
      auto external = ctx->isExtern != nullptr;
      auto proto = std::move(visitFnPrototype(ctx->fnPrototype()).as<ast::FnPrototype>());

      visitBlockExpression(ctx->blockExpression());
      auto body = gal::static_unique_cast<ast::BlockExpression>(return_value<ast::Expression>());

      RETURN(std::make_unique<ast::FnDeclaration>(loc_from(ctx->fnPrototype()),
          exported_,
          external,
          std::move(proto),
          std::move(body)));
    }

    antlrcpp::Any visitFnAttributeList(GalliumParser::FnAttributeListContext* ctx) final {
      std::vector<ast::Attribute> attributes;

      for (auto* attribute : ctx->fnAttribute()) {
        auto single_attribute = std::move(visitFnAttribute(attribute).as<ast::Attribute>());

        attributes.push_back(std::move(single_attribute));
      }

      return {std::move(attributes)};
    }

    antlrcpp::Any visitFnAttribute(GalliumParser::FnAttributeContext* ctx) final {
      using Type = ast::AttributeType;

      auto start = ctx->getStart()->getStartIndex();
      auto stop = ctx->getStop()->getStopIndex();
      auto attribute = original_.substr(start, (stop - start) + 1);

      if (attribute.find("__arch") != std::string::npos) {
        return ast::Attribute{Type::builtin_arch, {ctx->STRING_LITERAL()->toString()}};
      }

      static absl::flat_hash_map<std::string_view, Type> lookup{
          {"__pure", Type::builtin_pure},
          {"__throws", Type::builtin_throws},
          {"__alwaysinline", Type::builtin_always_inline},
          {"__inline", Type::builtin_inline},
          {"__noinline", Type::builtin_no_inline},
          {"__malloc", Type::builtin_malloc},
          {"__hot", Type::builtin_hot},
          {"__cold", Type::builtin_cold},
          {"__noreturn", Type::builtin_noreturn},
      };

      // will throw and end up crashing if one is ever not in the map
      return ast::Attribute{lookup.at(attribute), {}};
    }

    antlrcpp::Any visitFnArgumentList(GalliumParser::FnArgumentListContext* ctx) final {
      std::vector<ast::Argument> args;

      for (auto* arg : ctx->singleFnArgument()) {
        args.push_back(std::move(visitSingleFnArgument(arg).as<ast::Argument>()));
      }

      return {std::move(args)};
    }

    antlrcpp::Any visitSingleFnArgument(GalliumParser::SingleFnArgumentContext* ctx) final {
      visitType(ctx->type());

      auto name = ctx->IDENTIFIER()->toString();
      auto type = return_value<ast::Type>();

      return {ast::Argument{std::move(name), std::move(type)}};
    }

    antlrcpp::Any visitTypeParamList(GalliumParser::TypeParamListContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitTypeParam(GalliumParser::TypeParamContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitClassDeclaration(GalliumParser::ClassDeclarationContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitClassInheritance(GalliumParser::ClassInheritanceContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitClassInheritedType(GalliumParser::ClassInheritedTypeContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitClassMember(GalliumParser::ClassMemberContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitClassMemberWithoutPub(GalliumParser::ClassMemberWithoutPubContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitClassVariableBody(GalliumParser::ClassVariableBodyContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitStructDeclaration(GalliumParser::StructDeclarationContext* ctx) final {
      auto name = ctx->IDENTIFIER()->toString();
      auto fields = std::vector<ast::Field>{};

      for (auto* member : ctx->structMember()) {
        fields.push_back(std::move(visitStructMember(member).as<ast::Field>()));
      }

      RETURN(std::make_unique<ast::StructDeclaration>(loc_from(ctx), exported_, std::move(name), std::move(fields)));
    }

    antlrcpp::Any visitStructMember(GalliumParser::StructMemberContext* ctx) final {
      visitType(ctx->type());

      auto name = ctx->IDENTIFIER()->toString();
      auto type = return_value<ast::Type>();

      return ast::Field{loc_from(ctx), std::move(name), std::move(type)};
    }

    antlrcpp::Any visitTypeDeclaration(GalliumParser::TypeDeclarationContext* ctx) final {
      visitType(ctx->type());

      auto name = ctx->IDENTIFIER()->toString();
      auto type = return_value<ast::Type>();

      RETURN(std::make_unique<ast::TypeDeclaration>(loc_from(ctx), exported_, std::move(name), std::move(type)));
    }

    antlrcpp::Any visitStatement(GalliumParser::StatementContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitAssertStatement(GalliumParser::AssertStatementContext* ctx) final {
      auto condition = parse_expr(ctx->expr());
      auto lit = parse_string_lit(ctx, ctx->STRING_LITERAL());

      RETURN(std::make_unique<ast::AssertStatement>(loc_from(ctx),
          std::move(condition),
          gal::static_unique_cast<ast::StringLiteralExpression>(std::move(lit))));
    }

    antlrcpp::Any visitBindingStatement(GalliumParser::BindingStatementContext* ctx) final {
      auto name = ctx->IDENTIFIER()->toString();
      auto initializer = parse_expr(ctx->expr());
      auto mut = ctx->var != nullptr;
      auto type = (ctx->type() != nullptr) ? std::make_optional(parse_type(ctx->type())) : std::nullopt;

      RETURN(std::make_unique<ast::BindingStatement>(loc_from(ctx),
          std::move(name),
          mut,
          std::move(initializer),
          std::move(type)));
    }

    antlrcpp::Any visitExprStatement(GalliumParser::ExprStatementContext* ctx) final {
      RETURN(std::make_unique<ast::ExpressionStatement>(loc_from(ctx), parse_expr(ctx->expr())));
    }

    antlrcpp::Any visitCallArgList(GalliumParser::CallArgListContext* ctx) final {
      auto args = std::vector<std::unique_ptr<ast::Expression>>{};

      for (auto* expr : ctx->expr()) {
        args.push_back(parse_expr(expr));
      }

      return {std::move(args)};
    }

    antlrcpp::Any visitRestOfCall(GalliumParser::RestOfCallContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitBlockExpression(GalliumParser::BlockExpressionContext* ctx) final {
      auto statements = std::vector<std::unique_ptr<ast::Statement>>{};

      for (auto* stmt : ctx->statement()) {
        visitStatement(stmt);
        statements.push_back(return_value<ast::Statement>());
      }

      RETURN(std::make_unique<ast::BlockExpression>(loc_from(ctx), std::move(statements)));
    }

    antlrcpp::Any visitReturnExpr(GalliumParser::ReturnExprContext* ctx) final {
      std::optional<std::unique_ptr<ast::Expression>> expr;

      if (auto* ptr = ctx->expr()) {
        expr = parse_expr(ptr);
      }

      RETURN(std::make_unique<ast::ReturnExpression>(loc_from(ctx), std::move(expr)));
    }

    antlrcpp::Any visitBreakExpr(GalliumParser::BreakExprContext* ctx) final {
      std::optional<std::unique_ptr<ast::Expression>> expr;

      if (auto* ptr = ctx->expr()) {
        expr = parse_expr(ptr);
      }

      RETURN(std::make_unique<ast::BreakExpression>(loc_from(ctx), std::move(expr)));
    }

    antlrcpp::Any visitContinueExpr(GalliumParser::ContinueExprContext* ctx) final {
      RETURN(std::make_unique<ast::ContinueExpression>(loc_from(ctx)));
    }

    antlrcpp::Any visitIfExpr(GalliumParser::IfExprContext* ctx) final {
      if (ctx->blockExpression() != nullptr) {
        RETURN(parse_if_block(ctx));
      } else {
        RETURN(parse_if_then(ctx));
      }
    }

    antlrcpp::Any visitElifBlock(GalliumParser::ElifBlockContext* ctx) final {
      auto cond = parse_expr(ctx->expr());
      auto body = parse_block(ctx->blockExpression());

      return {ast::ElifBlock{std::move(cond), std::move(body)}};
    }

    antlrcpp::Any visitElseBlock(GalliumParser::ElseBlockContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitLoopExpr(GalliumParser::LoopExprContext* ctx) final {
      if (ctx->whileCond != nullptr) {
        auto condition = parse_expr(ctx->whileCond);
        auto body = parse_block(ctx->blockExpression());

        RETURN(std::make_unique<ast::WhileExpression>(loc_from(ctx), std::move(condition), std::move(body)));
      } else if (ctx->loopVariable != nullptr) {
        auto loop_var = ctx->IDENTIFIER()->toString();
        auto initializer = parse_expr(ctx->expr(0));
        auto until = parse_expr(ctx->expr(1));
        auto body = parse_block(ctx->blockExpression());
        auto direction =
            (ctx->direction->getType() == GalliumParser::TO) ? ast::ForDirection::up_to : ast::ForDirection::down_to;

        RETURN(std::make_unique<ast::ForExpression>(loc_from(ctx),
            std::move(loop_var),
            direction,
            std::move(initializer),
            std::move(until),
            std::move(body)));
      } else {
        auto body = parse_block(ctx->blockExpression());

        RETURN(std::make_unique<ast::LoopExpression>(loc_from(ctx), std::move(body)));
      }
    }

    antlrcpp::Any visitExpr(GalliumParser::ExprContext* ctx) final {
      if (ctx->op != nullptr || ctx->gtgtHack != nullptr) {
        RETURN(parse_binary_or_unary(ctx));
      } else if (ctx->restOfCall() != nullptr) {
        RETURN(parse_callish(ctx));
      } else if (ctx->as != nullptr || ctx->asUnsafe != nullptr) {
        RETURN(parse_cast(ctx));
      } else {
        return visitChildren(ctx);
      }
    }

    antlrcpp::Any visitPrimaryExpr(GalliumParser::PrimaryExprContext* ctx) final {
      if (ctx->STRING_LITERAL() != nullptr) {
        RETURN(parse_string_lit(ctx, ctx->STRING_LITERAL()));
      } else if (ctx->CHAR_LITERAL() != nullptr) {
        RETURN(parse_char_lit(ctx, ctx->CHAR_LITERAL()));
      } else if (ctx->BOOL_LITERAL() != nullptr) {
        RETURN(std::make_unique<ast::BoolLiteralExpression>(loc_from(ctx), ctx->BOOL_LITERAL()->getText() == "true"));
      } else if (ctx->NIL_LITERAL() != nullptr) {
        RETURN(std::make_unique<ast::NilLiteralExpression>(loc_from(ctx)));
      } else if (ctx->maybeGenericIdentifier() != nullptr) {
        auto id = std::move(visitMaybeGenericIdentifier(ctx->maybeGenericIdentifier()).as<ast::UnqualifiedID>());
        RETURN(std::make_unique<ast::UnqualifiedIdentifierExpression>(loc_from(ctx),
            std::move(id),
            std::vector<std::unique_ptr<ast::Type>>{},
            std::nullopt));
      } else {
        return visitChildren(ctx);
      }
    }

    antlrcpp::Any visitStructInitExpr(GalliumParser::StructInitExprContext* ctx) final {
      auto type = parse_type(ctx->typeWithoutRef());
      auto list =
          std::move(visitStructInitMemberList(ctx->structInitMemberList()).as<std::vector<ast::FieldInitializer>>());

      RETURN(std::make_unique<ast::StructExpression>(loc_from(ctx), std::move(type), std::move(list)));
    }

    antlrcpp::Any visitStructInitMemberList(GalliumParser::StructInitMemberListContext* ctx) final {
      auto result = std::vector<ast::FieldInitializer>{};

      for (auto* init_member : ctx->structInitMember()) {
        result.push_back(std::move(visitStructInitMember(init_member).as<ast::FieldInitializer>()));
      }

      return result;
    }

    antlrcpp::Any visitStructInitMember(GalliumParser::StructInitMemberContext* ctx) final {
      auto name = ctx->IDENTIFIER()->getText();
      auto expr = parse_expr(ctx->expr());

      return ast::FieldInitializer(loc_from(ctx), std::move(name), std::move(expr));
    }

    antlrcpp::Any visitConstantExpr(GalliumParser::ConstantExprContext* ctx) final {
      if (ctx->STRING_LITERAL() != nullptr) {
        RETURN(parse_string_lit(ctx, ctx->STRING_LITERAL()));
      } else if (ctx->CHAR_LITERAL() != nullptr) {
        RETURN(parse_char_lit(ctx, ctx->CHAR_LITERAL()));
      } else if (ctx->BOOL_LITERAL() != nullptr) {
        RETURN(std::make_unique<ast::BoolLiteralExpression>(loc_from(ctx), ctx->BOOL_LITERAL()->getText() == "true"));
      } else if (ctx->NIL_LITERAL() != nullptr) {
        RETURN(std::make_unique<ast::NilLiteralExpression>(loc_from(ctx)));
      } else {
        return visitChildren(ctx);
      }
    }

    antlrcpp::Any visitGroupExpr(GalliumParser::GroupExprContext* ctx) final {
      RETURN(std::make_unique<ast::GroupExpression>(loc_from(ctx), parse_expr(ctx->expr())));
    }

    antlrcpp::Any visitDigitLiteral(GalliumParser::DigitLiteralContext* ctx) final {
      auto digits = std::string{};
      auto base = int{};

      if (ctx->hex != nullptr) {
        digits = ctx->HEX_LITERAL()->toString().substr(2);
        base = 16;
      } else if (ctx->octal != nullptr) {
        digits = ctx->OCTAL_LITERAL()->toString().substr(2);
        base = 8;
      } else if (ctx->binary != nullptr) {
        digits = ctx->BINARY_LITERAL()->toString().substr(2);
        base = 2;
      } else {
        digits = ctx->DECIMAL_LITERAL()->toString();
        base = 10;
      }

      auto value = parse_value<std::uint64_t>(digits, base, "integer literal");

      if (auto* int_value = std::get_if<std::uint64_t>(&value)) {
        RETURN(std::make_unique<ast::IntegerLiteralExpression>(loc_from(ctx), *int_value));
      } else {
        RETURN(error_expr(3, loc_from(ctx), {std::get<std::string>(value)}));
      }
    }

    antlrcpp::Any visitFloatLiteral(GalliumParser::FloatLiteralContext* ctx) final {
      auto literals = ctx->DECIMAL_LITERAL();
      auto as_string = (literals.size() == 2) ? absl::StrCat(literals[0]->toString(), ".", literals[1]->toString())
                                              : absl::StrCat("0.", literals[0]->toString());

      auto result = gal::from_digits(as_string, std::chars_format::general);

      if (auto* error = std::get_if<std::error_code>(&result)) {
        RETURN(error_expr(4, loc_from(ctx), {error->message()}));
      }

      RETURN(
          std::make_unique<ast::FloatLiteralExpression>(loc_from(ctx), std::get<double>(result), as_string.length()));
    }

    antlrcpp::Any visitMaybeGenericIdentifier(GalliumParser::MaybeGenericIdentifierContext* ctx) final {
      auto id = std::move(visitModularIdentifier(ctx->modularIdentifier()).as<ast::ModuleID>());
      auto unqualified_id = ast::module_into_unqualified(std::move(id));

      return {std::move(unqualified_id)};
    }

    antlrcpp::Any visitType(GalliumParser::TypeContext* ctx) final {
      auto* ref = ctx->ref;

      if (ref == nullptr) {
        return visitTypeWithoutRef(ctx->typeWithoutRef());
      }

      auto mut = ref->getType() == GalliumLexer::AMPERSTAND_MUT;
      auto referenced = parse_type(ctx->typeWithoutRef());

      RETURN(std::make_unique<ast::ReferenceType>(loc_from(ctx), mut, std::move(referenced)));
    }

    antlrcpp::Any visitTypeWithoutRef(GalliumParser::TypeWithoutRefContext* ctx) final {
      if (ctx->squareBracket != nullptr) {
        auto type = parse_type(ctx->typeWithoutRef());

        RETURN(std::make_unique<ast::SliceType>(loc_from(ctx), std::move(type)));
      } else if (ctx->ptr != nullptr) {
        auto type = parse_type(ctx->typeWithoutRef());

        // can have `ptr` with either STAR_MUT or STAR_CONST
        RETURN(std::make_unique<ast::PointerType>(loc_from(ctx),
            ctx->ptr->getType() == GalliumParser::STAR_MUT,
            std::move(type)));
      } else if (ctx->BUILTIN_TYPE() != nullptr) {
        RETURN(parse_builtin(ctx));
      }

      auto generic_list = parse_type_list(ctx->genericTypeList());

      if (ctx->fnType != nullptr) {
        RETURN(std::make_unique<ast::FnPointerType>(loc_from(ctx), std::move(generic_list), parse_type(ctx->type())));
      }

      auto id = std::move(visitMaybeGenericIdentifier(ctx->maybeGenericIdentifier()).as<ast::UnqualifiedID>());

      if (ctx->userDefinedType != nullptr) {
        RETURN(
            std::make_unique<ast::UnqualifiedUserDefinedType>(loc_from(ctx), std::move(id), std::move(generic_list)));
      } else {
        RETURN(
            std::make_unique<ast::UnqualifiedDynInterfaceType>(loc_from(ctx), std::move(id), std::move(generic_list)));
      }
    }

    antlrcpp::Any visitGenericTypeList(GalliumParser::GenericTypeListContext* ctx) final {
      auto list = std::vector<std::unique_ptr<ast::Type>>{};

      for (auto* type : ctx->type()) {
        list.push_back(parse_type(type));
      }

      return {std::move(list)};
    }

  private:
    // can deal with null context
    std::vector<std::unique_ptr<ast::Type>> parse_type_list(GalliumParser::GenericTypeListContext* ctx) {
      if (ctx != nullptr) {
        return std::move(visitGenericTypeList(ctx).as<std::vector<std::unique_ptr<ast::Type>>>());
      }

      return {};
    }

    std::unique_ptr<ast::Type> parse_builtin(GalliumParser::TypeWithoutRefContext* ctx) noexcept {
      auto as_string = ctx->BUILTIN_TYPE()->toString();

      if (as_string == "bool") {
        return std::make_unique<ast::BuiltinBoolType>(loc_from(ctx));
      } else if (as_string == "byte") {
        return std::make_unique<ast::BuiltinByteType>(loc_from(ctx));
      } else if (as_string == "char") {
        return std::make_unique<ast::BuiltinCharType>(loc_from(ctx));
      } else if (as_string == "void") {
        return std::make_unique<ast::VoidType>(loc_from(ctx));
      }

      assert(as_string[0] == 'i' || as_string[0] == 'u' || as_string[0] == 'f');

      // split into ['', width]
      auto [prefix, rest] = std::pair{as_string.substr(0, 1), as_string.substr(1)};

      if (rest == "size") {
        return std::make_unique<ast::BuiltinIntegralType>(loc_from(ctx),
            as_string[0] == 'i',
            ast::IntegerWidth::native_width);
      }

      auto result = gal::from_digits(rest, 10);

      if (auto* error = std::get_if<std::error_code>(&result)) {
        return error_type(1,
            loc_from(ctx),
            {absl::StrCat("error from integer parser: '", absl::StripAsciiWhitespace(error->message()), "'")});
      }

      auto real_width = std::get<std::uint64_t>(result);

      if (as_string[0] == 'f') {
        if (real_width != 32 && real_width != 64 && real_width != 128) {
          return error_type(1, loc_from(ctx));
        }

        auto float_width = (real_width == 32)   ? ast::FloatWidth::ieee_single
                           : (real_width == 64) ? ast::FloatWidth::ieee_double
                                                : ast::FloatWidth::ieee_quadruple;

        return std::make_unique<ast::BuiltinFloatType>(loc_from(ctx), float_width);
      }

      if (real_width == 8 || real_width == 16 || real_width == 32 || real_width == 64 || real_width == 128) {
        return std::make_unique<ast::BuiltinIntegralType>(loc_from(ctx),
            as_string[0] == 'i',
            static_cast<ast::IntegerWidth>(real_width));
      }

      return error_type(1, loc_from(ctx));
    }

    static ast::UnaryOp unary_op(antlr4::Token* op) noexcept {
      switch (op->getType()) {
        case GalliumParser::NOT_KEYWORD: return ast::UnaryOp::logical_not;
        case GalliumParser::TILDE: return ast::UnaryOp::bitwise_not;
        case GalliumParser::AMPERSTAND: return ast::UnaryOp::ref_to;
        case GalliumParser::AMPERSTAND_MUT: return ast::UnaryOp::mut_ref_to;
        case GalliumParser::HYPHEN: return ast::UnaryOp::negate;
        case GalliumParser::STAR: return ast::UnaryOp::dereference;
        default: assert(false); return ast::UnaryOp::bitwise_not;
      }
    }

    static ast::BinaryOp binary_op(antlr4::Token* op, bool gtgt_hack) noexcept {
      if (gtgt_hack) {
        return ast::BinaryOp::right_shift;
      }

      switch (op->getType()) {
        case GalliumParser::STAR: return ast::BinaryOp::mul;
        case GalliumParser::FORWARD_SLASH: return ast::BinaryOp::div;
        case GalliumParser::PERCENT: return ast::BinaryOp::mod;
        case GalliumParser::PLUS: return ast::BinaryOp::add;
        case GalliumParser::HYPHEN: return ast::BinaryOp::sub;
        case GalliumParser::LTLT: return ast::BinaryOp::left_shift;
        case GalliumParser::AMPERSTAND: return ast::BinaryOp::bitwise_and;
        case GalliumParser::CARET: return ast::BinaryOp::bitwise_xor;
        case GalliumParser::PIPE: return ast::BinaryOp::bitwise_or;
        case GalliumParser::AND_KEYWORD: return ast::BinaryOp::logical_and;
        case GalliumParser::XOR_KEYWORD: return ast::BinaryOp::logical_xor;
        case GalliumParser::OR_KEYWORD: return ast::BinaryOp::logical_or;
        case GalliumParser::LT: return ast::BinaryOp::lt;
        case GalliumParser::GT: return ast::BinaryOp::gt;
        case GalliumParser::LTEQ: return ast::BinaryOp::lt_eq;
        case GalliumParser::GTEQ: return ast::BinaryOp::gt_eq;
        case GalliumParser::EQEQ: return ast::BinaryOp::equals;
        case GalliumParser::BANGEQ: return ast::BinaryOp::not_equal;
        case GalliumParser::WALRUS: return ast::BinaryOp::assignment;
        case GalliumParser::PLUSEQ: return ast::BinaryOp::add_eq;
        case GalliumParser::HYPHENEQ: return ast::BinaryOp::sub_eq;
        case GalliumParser::STAREQ: return ast::BinaryOp::mul_eq;
        case GalliumParser::SLASHEQ: return ast::BinaryOp::div_eq;
        case GalliumParser::PERCENTEQ: return ast::BinaryOp::mod_eq;
        case GalliumParser::LTLTEQ: return ast::BinaryOp::left_shift_eq;
        case GalliumParser::GTGTEQ: return ast::BinaryOp::right_shift_eq;
        case GalliumParser::AMPERSTANDEQ: return ast::BinaryOp::bitwise_and_eq;
        case GalliumParser::CARETEQ: return ast::BinaryOp::bitwise_xor_eq;
        case GalliumParser::PIPEEQ: return ast::BinaryOp::bitwise_or_eq;
        default: assert(false); return ast::BinaryOp::mul;
      }
    }

    std::unique_ptr<ast::Expression> parse_binary_or_unary(GalliumParser::ExprContext* ctx) noexcept {
      auto exprs = ctx->expr();
      auto first = parse_expr(exprs.front());

      if (exprs.size() == 1) {
        auto op = unary_op(ctx->op);

        return std::make_unique<ast::UnaryExpression>(loc_from(ctx), op, std::move(first));
      } else {
        assert(exprs.size() == 2);

        auto second = parse_expr(exprs.back());
        auto op = binary_op(ctx->op, ctx->gtgtHack != nullptr);

        return std::make_unique<ast::BinaryExpression>(loc_from(ctx), op, std::move(first), std::move(second));
      }
    }

    std::unique_ptr<ast::Expression> parse_callish(GalliumParser::ExprContext* ctx) noexcept {
      auto* rest = ctx->restOfCall();
      auto callee = parse_expr(ctx->expr(0));

      // if both of those aren't there, we can only be a field access expr
      if (rest->paren == nullptr && rest->bracket == nullptr) {
        return std::make_unique<ast::FieldAccessExpression>(loc_from(ctx),
            std::move(callee),
            rest->IDENTIFIER()->toString());
      }

      if (rest->paren != nullptr && rest->callArgList() == nullptr) {
        return std::make_unique<ast::CallExpression>(loc_from(ctx),
            std::move(callee),
            std::vector<std::unique_ptr<ast::Expression>>{},
            std::vector<std::unique_ptr<ast::Type>>{});
      }

      if (rest->bracket != nullptr && rest->callArgList() == nullptr) {
        return std::make_unique<ast::IndexExpression>(loc_from(ctx),
            std::move(callee),
            std::vector<std::unique_ptr<ast::Expression>>{});
      }

      auto args = std::move(visitCallArgList(rest->callArgList()) //
                                .as<std::vector<std::unique_ptr<ast::Expression>>>());

      if (rest->paren != nullptr) {
        return std::make_unique<ast::CallExpression>(loc_from(ctx),
            std::move(callee),
            std::move(args),
            std::vector<std::unique_ptr<ast::Type>>{});
      } else {
        return std::make_unique<ast::IndexExpression>(loc_from(ctx), std::move(callee), std::move(args));
      }
    }

    std::unique_ptr<ast::CastExpression> parse_cast(GalliumParser::ExprContext* ctx) noexcept {
      auto unsafe = ctx->asUnsafe != nullptr;
      auto casting_to = parse_type(ctx->type());
      auto castee = parse_expr(ctx->expr(0));

      return std::make_unique<ast::CastExpression>(loc_from(ctx), unsafe, std::move(castee), std::move(casting_to));
    }

    std::unique_ptr<ast::IfThenExpression> parse_if_then(GalliumParser::IfExprContext* ctx) noexcept {
      // [0] = cond, [1] = true-branch, [2] = false-branch
      auto exprs = std::array<std::unique_ptr<ast::Expression>, 3>{};
      auto i = 0;

      for (auto* expr : ctx->expr()) {
        exprs[i++] = parse_expr(expr);
      }

      return std::make_unique<ast::IfThenExpression>(loc_from(ctx),
          std::move(exprs[0]),
          std::move(exprs[1]),
          std::move(exprs[2]));
    }

    std::unique_ptr<ast::IfElseExpression> parse_if_block(GalliumParser::IfExprContext* ctx) noexcept {
      auto condition = parse_expr(ctx->expr(0));
      auto body = parse_block(ctx->blockExpression());
      auto elifs = parse_elifs(ctx);
      auto else_block = parse_else_block(ctx->elseBlock());

      return std::make_unique<ast::IfElseExpression>(loc_from(ctx),
          std::move(condition),
          std::move(body),
          std::move(elifs),
          std::move(else_block));
    }

    std::optional<std::unique_ptr<ast::BlockExpression>> parse_else_block(
        GalliumParser::ElseBlockContext* ctx) noexcept {
      if (ctx != nullptr) {
        return std::make_optional(parse_block(ctx->blockExpression()));
      }

      return std::nullopt;
    }

    std::vector<ast::ElifBlock> parse_elifs(GalliumParser::IfExprContext* ctx) noexcept {
      auto elifs = std::vector<ast::ElifBlock>{};

      for (auto* elif : ctx->elifBlock()) {
        auto elif_block = std::move(visitElifBlock(elif).as<ast::ElifBlock>());

        elifs.push_back(std::move(elif_block));
      }

      return elifs;
    }

    std::unique_ptr<ast::Expression> parse_string_lit(antlr4::ParserRuleContext* ctx,
        antlr4::tree::TerminalNode* lit) noexcept {
      static_assert(std::numeric_limits<char>::is_signed ? std::numeric_limits<char>::digits == 7
                                                         : std::numeric_limits<char>::digits == 8);
      auto full = lit->toString();
      auto result = std::string{};
      auto curr = 0;

      for (auto it = full.begin(); it != full.end(); ++it, ++curr) {
        if (*it != '\\') {
          result += *it;

          continue;
        }

        auto next = *(it + 1);

        auto len = (next == 'o')                         ? 3  // octal = 3 digits
                   : (next == 'x')                       ? 2  // hex = 2 digits
                   : (std::isdigit(next) && next != '0') ? 3  // decimal = 3 digits, but \0 is 1
                                                         : 1; // all others are 1

        // len + 1 because want to take current as well as `len` next chars
        auto single_char = std::string_view{result}.substr(curr, len + 1);
        auto parsed_single = parse_single_char(single_char);

        if (auto* ptr = std::get_if<std::uint8_t>(&parsed_single)) {
          result += static_cast<char>(*ptr);
        } else {
          return error_expr(2, loc_from(ctx), {std::get<std::string>(parsed_single)});
        }

        // ensure that we actually skip the characters we parsed
        it += len;
      }

      return std::make_unique<ast::StringLiteralExpression>(loc_from(ctx), std::move(full));
    }

    template <typename ResultType>
    static std::variant<ResultType, std::string> parse_value(std::string_view digits,
        int base,
        std::string_view gallium_type) noexcept {
      static_assert(std::is_unsigned_v<ResultType>);

      auto result = gal::from_digits(digits, base);

      if (auto* error = std::get_if<std::error_code>(&result)) {
        return error->message();
      }

      auto value = std::get<std::uint64_t>(result);

      if (auto converted = gal::try_narrow<ResultType>(value)) {
        return *converted;
      }

      // while we could make `from_chars` do out-of-range checking,
      // we want a nice error message in the normal case of "slightly out of range"
      // it will still do out-of-range checking but will give a much worse
      // error message when it's out of bounds for 64-bit
      return absl::StrCat("value '",
          digits,
          "' parse to `",
          value,
          "` which is outside the range for a `",
          gallium_type,
          "` literal");
    }

    static std::variant<std::uint8_t, std::string> parse_single_char(std::string_view full) noexcept {
      if (full[0] != '\\') {
        return static_cast<std::uint8_t>(full[1]);
      }

      if (full[1] == 'o' || full[1] == 'x' || std::isdigit(full[1])) {
        auto digits = std::string_view{full}.substr(2);
        auto result = parse_value<std::uint8_t>(digits, full[1] == 'o' ? 8 : full[1] == 'x' ? 16 : 10, "char");

        if (std::holds_alternative<std::string>(result)) {
          return std::move(std::get<std::string>(result));
        }

        return std::get<std::uint8_t>(result);
      }

      // we can't do two implicit conversions when returning
      // to a `std::variant`, so its either that or `return static_cast<std::uint8_t>(c)`
      auto value = std::uint8_t{0};

      switch (full[1]) {
        case '0': value = '\0'; break;
        case 'n': value = '\n'; break;
        case 'r': value = '\r'; break;
        case 't': value = '\t'; break;
        case 'v': value = '\v'; break;
        case '\\': value = '\\'; break;
        case '"': value = '"'; break;
        case '\'': value = '\''; break;
        case 'a': value = '\a'; break;
        case 'b': value = '\b'; break;
        case 'f': value = '\f'; break;
        default: assert(false); break; // should be impossible with the grammar
      }

      return value;
    }

    std::unique_ptr<ast::Expression> parse_char_lit(antlr4::ParserRuleContext* ctx,
        antlr4::tree::TerminalNode* lit) noexcept {
      auto full = lit->toString();
      auto parsed = parse_single_char(full);

      if (auto* ptr = std::get_if<std::string>(&parsed)) {
        return error_expr(2, loc_from(ctx), {*ptr});
      }

      return std::make_unique<ast::CharLiteralExpression>(loc_from(ctx), std::get<std::uint8_t>(parsed));
    }

    std::unique_ptr<ast::Expression> parse_expr(GalliumParser::ExprContext* ctx) noexcept {
      visitExpr(ctx);

      return return_value<ast::Expression>();
    }

    std::unique_ptr<ast::Type> parse_type(GalliumParser::TypeContext* ctx) noexcept {
      visitType(ctx);

      return return_value<ast::Type>();
    }

    std::unique_ptr<ast::Type> parse_type(GalliumParser::TypeWithoutRefContext* ctx) noexcept {
      visitTypeWithoutRef(ctx);

      return return_value<ast::Type>();
    }

    std::unique_ptr<ast::BlockExpression> parse_block(GalliumParser::BlockExpressionContext* ctx) noexcept {
      visitBlockExpression(ctx);

      return gal::static_unique_cast<ast::BlockExpression>(return_value<ast::Expression>());
    }

    antlrcpp::Any ret(std::unique_ptr<ast::Declaration> ptr) noexcept {
      decl_ret_ = std::move(ptr);

      return {};
    }

    antlrcpp::Any ret(std::unique_ptr<ast::Statement> ptr) noexcept {
      stmt_ret_ = std::move(ptr);

      return {};
    }

    antlrcpp::Any ret(std::unique_ptr<ast::Expression> ptr) noexcept {
      expr_ret_ = std::move(ptr);

      return {};
    }

    antlrcpp::Any ret(std::unique_ptr<ast::Type> ptr) noexcept {
      type_ret_ = std::move(ptr);

      return {};
    }

    template <typename T> std::unique_ptr<T> return_value() noexcept {
      // hack to simplify getting the return value from a "returning" visitThing() call
      if constexpr (std::is_same_v<T, ast::Declaration>) {
        assert(decl_ret_ != nullptr);

        return std::move(decl_ret_);
      } else if constexpr (std::is_same_v<T, ast::Expression>) {
        assert(expr_ret_ != nullptr);

        return std::move(expr_ret_);
      } else if constexpr (std::is_same_v<T, ast::Statement>) {
        assert(stmt_ret_ != nullptr);

        return std::move(stmt_ret_);
      } else if constexpr (std::is_same_v<T, ast::Type>) {
        assert(type_ret_ != nullptr);

        return std::move(type_ret_);
      } else {
        static_assert(!sizeof(T*), "check your types");
      }
    }

    void push_error(std::int64_t code,
        std::vector<gal::PointedOut> locs,
        absl::Span<const std::string> notes = {}) noexcept {
      auto underline = std::make_unique<gal::UnderlineList>(std::move(locs));
      auto vec = std::vector<std::unique_ptr<gal::DiagnosticPart>>{};

      vec.push_back(std::move(underline));

      for (auto& s : notes) {
        vec.push_back(std::make_unique<gal::SingleMessage>(s, gal::DiagnosticType::note));
      }

      diagnostics_->report_emplace(code, std::move(vec));
    }

    std::unique_ptr<ast::Type> error_type(std::int64_t code,
        ast::SourceLoc loc,
        absl::Span<const std::string> notes = {}) noexcept {
      push_error(code, {{loc}}, notes);

      return std::make_unique<ast::ErrorType>();
    }

    std::unique_ptr<ast::Expression> error_expr(std::int64_t code,
        ast::SourceLoc loc,
        absl::Span<const std::string> notes = {}) noexcept {
      push_error(code, {{loc}}, notes);

      return std::make_unique<ast::ErrorExpression>();
    }

    std::unique_ptr<ast::Declaration> error_decl(std::int64_t code, ast::SourceLoc loc) noexcept {
      push_error(code, {{loc}});

      return std::make_unique<ast::ErrorDeclaration>();
    }

    template <typename T> ast::SourceLoc loc_from(T* node) noexcept {
      auto* firstToken = node->getStart();

      return ast::SourceLoc(node->getText(), firstToken->getLine(), firstToken->getCharPositionInLine(), path_);
    }

    std::unique_ptr<ast::Declaration> decl_ret_;
    std::unique_ptr<ast::Statement> stmt_ret_;
    std::unique_ptr<ast::Expression> expr_ret_;
    std::unique_ptr<ast::Type> type_ret_;
    gal::DiagnosticReporter* diagnostics_;
    std::filesystem::path path_;
    std::string_view original_;
    bool exported_ = false;
  };
} // namespace

namespace gal {
  std::optional<ast::Program> parse(std::filesystem::path path,
      std::string_view source_code,
      gal::DiagnosticReporter* reporter) noexcept {
    auto input = antlr4::ANTLRInputStream(std::string{source_code});
    auto error_handler = gal::ParserErrorListener{path, reporter};
    auto lex = GalliumLexer(&input);
    lex.removeErrorListeners();
    lex.addErrorListener(&error_handler);
    auto tokens = antlr4::CommonTokenStream(&lex);
    auto parser = GalliumParser(&tokens);
    parser.removeErrorListeners();
    parser.addErrorListener(&error_handler);
    auto* tree = parser.parse();

    if (parser.getNumberOfSyntaxErrors() != 0) {
      return std::nullopt;
    }

    return ASTGenerator(reporter).into_ast(source_code, std::move(path), tree);
  }
} // namespace gal