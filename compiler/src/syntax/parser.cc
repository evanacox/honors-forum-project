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
#include "../core/error_reporting.h"
#include "../utility/misc.h"
#include "absl/container/flat_hash_map.h"
#include "absl/strings/str_split.h"
#include "antlr4-runtime.h"
#include "generated/GalliumBaseVisitor.h"
#include "generated/GalliumLexer.h"
#include "generated/GalliumParser.h"
#include <charconv>
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

  template <typename T> ast::SourceLoc loc_from(T* node) noexcept {
    auto* firstToken = node->getStart();

    return ast::SourceLoc(node->getText(), firstToken->getLine(), firstToken->getCharPositionInLine(), "");
  }

  class ASTGenerator final : public GalliumBaseVisitor {
  public:
    ast::Program into_ast(GalliumParser::ParseContext* parse_tree) noexcept {
      std::vector<std::unique_ptr<ast::Declaration>> decls;

      for (auto* decl : parse_tree->modularizedDeclaration()) {
        visitModularizedDeclaration(decl);

        decls.push_back(return_value<ast::Declaration>());
      }

      return ast::Program(std::move(decls));
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
          ids.emplace_back(module_id, id->toString());
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

    antlrcpp::Any visitFnDeclaration(GalliumParser::FnDeclarationContext* ctx) final {
      std::vector<ast::Argument> args;
      if (auto* list = ctx->fnPrototype()->fnArgumentList()) {
        args = std::move(visitFnArgumentList(list).as<decltype(args)>());
      }

      std::vector<ast::Attribute> attributes;
      if (auto* list = ctx->fnPrototype()->fnAttributeList()) {
        attributes = std::move(visitFnAttributeList(list).as<decltype(attributes)>());
      }

      auto external = ctx->isExtern != nullptr;
      auto name = ctx->fnPrototype()->IDENTIFIER()->toString();

      visitType(ctx->fnPrototype()->type());
      auto return_type = return_value<ast::Type>();

      visitBlockExpression(ctx->blockExpression());
      auto body = gal::static_unique_cast<ast::BlockExpression>(return_value<ast::Expression>());

      return std::make_unique<ast::FnDeclaration>(loc_from(ctx->fnPrototype()),
          exported_,
          external,
          ast::FnPrototype(std::move(name),
              std::nullopt,
              std::move(args),
              std::move(attributes),
              std::move(return_type)),
          std::move(body));
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

      auto attribute = ctx->toString();

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
      return visitChildren(ctx);
    }

    antlrcpp::Any visitStructMember(GalliumParser::StructMemberContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitTypeDeclaration(GalliumParser::TypeDeclarationContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitStatement(GalliumParser::StatementContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitAssertStatement(GalliumParser::AssertStatementContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitBindingStatement(GalliumParser::BindingStatementContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitExprStatement(GalliumParser::ExprStatementContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitExprList(GalliumParser::ExprListContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitRestOfCall(GalliumParser::RestOfCallContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitBlockExpression(GalliumParser::BlockExpressionContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitReturnExpr(GalliumParser::ReturnExprContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitIfExpr(GalliumParser::IfExprContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitLoopExpr(GalliumParser::LoopExprContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitExpr(GalliumParser::ExprContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitPrimaryExpr(GalliumParser::PrimaryExprContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitGroupExpr(GalliumParser::GroupExprContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitDigitLiteral(GalliumParser::DigitLiteralContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitType(GalliumParser::TypeContext* ctx) final {
      auto* ref = ctx->ref;

      if (ref == nullptr) {
        return visitTypeWithoutRef(ctx->typeWithoutRef());
      }

      auto mut = ref->getType() == GalliumLexer::AMPERSTAND_MUT;
      visitTypeWithoutRef(ctx->typeWithoutRef());
      auto referenced = return_value<ast::Type>();

      RETURN(std::make_unique<ast::ReferenceType>(loc_from(ctx), mut, std::move(referenced)));
    }

    antlrcpp::Any visitTypeWithoutRef(GalliumParser::TypeWithoutRefContext* ctx) final {
      if (ctx->squareBracket != nullptr) {
        visitTypeWithoutRef(ctx->typeWithoutRef());

        RETURN(std::make_unique<ast::SliceType>(loc_from(ctx), return_value<ast::Type>()));
      } else if (ctx->ptr != nullptr) {
        visitTypeWithoutRef(ctx->typeWithoutRef());

        // can have `ptr` with either STAR_MUT or STAR_CONST
        RETURN(std::make_unique<ast::PointerType>(loc_from(ctx),
            ctx->ptr->getType() == GalliumParser::STAR_MUT,
            return_value<ast::Type>()));
      } else if (ctx->BUILTIN_TYPE() != nullptr) {
        return parse_builtin(ctx);
      }

      auto generic_list = parse_type_list(ctx->genericTypeList());

      if (ctx->fnType != nullptr) {
        visitType(ctx->type());

        RETURN(std::make_unique<ast::FnPointerType>(loc_from(ctx), std::move(generic_list), return_value<ast::Type>()));
      }

      auto id = parse_unqual_id(ctx->modularIdentifier());

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
        visitType(type);

        list.push_back(return_value<ast::Type>());
      }

      return list;
    }

  private:
    static ast::UnqualifiedID parse_unqual_id(GalliumParser::ModularIdentifierContext* ctx) noexcept {
      std::vector<std::string> module_parts;

      for (auto* node : ctx->IDENTIFIER()) {
        module_parts.push_back(node->toString());
      }

      auto first = std::move(module_parts.front());
      module_parts.erase(module_parts.begin());

      return ast::UnqualifiedID(ast::ModuleID{ctx->isRoot != nullptr, std::move(module_parts)}, std::move(first));
    }

    // can deal with null context
    std::vector<std::unique_ptr<ast::Type>> parse_type_list(GalliumParser::GenericTypeListContext* ctx) {
      if (ctx != nullptr) {
        return std::move(visitGenericTypeList(ctx).as<std::vector<std::unique_ptr<ast::Type>>>());
      }

      return {};
    }

    antlrcpp::Any parse_builtin(GalliumParser::TypeWithoutRefContext* ctx) {
      auto as_string = ctx->BUILTIN_TYPE()->toString();

      if (as_string == "bool") {
        RETURN(std::make_unique<ast::BuiltinBoolType>(loc_from(ctx)));
      } else if (as_string == "byte") {
        RETURN(std::make_unique<ast::BuiltinByteType>(loc_from(ctx)));
      } else if (as_string == "char") {
        RETURN(std::make_unique<ast::BuiltinCharType>(loc_from(ctx)));
      } else if (as_string == "void") {
        RETURN(std::make_unique<ast::VoidType>(loc_from(ctx)));
      }

      assert(as_string[0] == 'i' || as_string[0] == 'u' || as_string[0] == 'f');

      // split into ['', width]
      std::vector<std::string_view> width = absl::StrSplit(as_string, as_string[0]);
      auto real_width = std::int64_t{0};
      auto [_, err] = std::from_chars(width[1].data(), width[1].data() + width[1].size(), real_width);

      if (err != std::errc()) {
        return error_expr(1, loc_from(ctx));
      }

      if (as_string[0] == 'f') {
        if (real_width != 32 && real_width != 64 && real_width != 128) {
          return error_expr(1, loc_from(ctx));
        }

        auto float_width = (real_width == 32)   ? ast::FloatWidth::ieee_single
                           : (real_width == 64) ? ast::FloatWidth::ieee_double
                                                : ast::FloatWidth::ieee_quadruple;

        RETURN(std::make_unique<ast::BuiltinFloatType>(loc_from(ctx), float_width));
      }

      if (real_width == 8 || real_width == 16 || real_width == 32 || real_width == 64 || real_width == 128) {
        RETURN(std::make_unique<ast::BuiltinIntegralType>(loc_from(ctx),
            as_string[0] == 'i',
            static_cast<ast::IntegerWidth>(real_width)));
      }

      return error_expr(1, loc_from(ctx));
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
        return std::move(decl_ret_);
      } else if constexpr (std::is_same_v<T, ast::Expression>) {
        return std::move(expr_ret_);
      } else if constexpr (std::is_same_v<T, ast::Statement>) {
        return std::move(stmt_ret_);
      } else if constexpr (std::is_same_v<T, ast::Type>) {
        return std::move(type_ret_);
      } else {
        static_assert(!sizeof(T*), "check your types");
      }
    }

    void push_error(std::int64_t code, std::vector<gal::UnderlineList::PointedOut> locs) noexcept {
      auto underline = std::make_unique<gal::UnderlineList>(std::move(locs));
      auto vec = std::vector<std::unique_ptr<gal::DiagnosticPart>>{};

      vec.push_back(std::move(underline));
      diagnostics_.emplace_back(code, std::move(vec));
    }

    antlrcpp::Any error_type(std::int64_t code, ast::SourceLoc loc) noexcept {
      push_error(code, {{loc}});

      return ret(std::make_unique<ast::ErrorType>());
    }

    antlrcpp::Any error_expr(std::int64_t code, ast::SourceLoc loc) noexcept {
      push_error(code, {{loc}});

      return ret(std::make_unique<ast::ErrorExpression>());
    }

    antlrcpp::Any error_decl(std::int64_t code, ast::SourceLoc loc) noexcept {
      push_error(code, {{loc}});

      return ret(std::make_unique<ast::ErrorDeclaration>());
    }

    std::unique_ptr<ast::Declaration> decl_ret_;
    std::unique_ptr<ast::Statement> stmt_ret_;
    std::unique_ptr<ast::Expression> expr_ret_;
    std::unique_ptr<ast::Type> type_ret_;
    std::vector<gal::Diagnostic> diagnostics_;
    bool exported_ = false;
  };
} // namespace

namespace gal {
  std::optional<ast::Program> parse(std::string_view source_code) noexcept {
    auto input = antlr4::ANTLRInputStream(source_code);
    auto lex = GalliumLexer(&input);
    auto tokens = antlr4::CommonTokenStream(&lex);
    auto parser = GalliumParser(&tokens);
    auto* tree = parser.parse();

    if (parser.getNumberOfSyntaxErrors() == 0) {
      auto walker = ASTGenerator();

      return walker.into_ast(tree);
    }

    return std::nullopt;
  }
} // namespace gal