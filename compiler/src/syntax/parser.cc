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
#include "antlr4-runtime.h"
#include "generated/GalliumBaseVisitor.h"
#include "generated/GalliumLexer.h"
#include "generated/GalliumParser.h"
#include <optional>
#include <string_view>

namespace {
  class ASTGenerator final : public GalliumVisitor {
    antlrcpp::Any visitParse(GalliumParser::ParseContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitModularIdentifier(GalliumParser::ModularIdentifierContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitModularizedDeclaration(GalliumParser::ModularizedDeclarationContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitImportDeclaration(GalliumParser::ImportDeclarationContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitIdentifierList(GalliumParser::IdentifierListContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitExportDeclaration(GalliumParser::ExportDeclarationContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitDeclaration(GalliumParser::DeclarationContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitFnDeclaration(GalliumParser::FnDeclarationContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitFnAttributeList(GalliumParser::FnAttributeListContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitFnAttribute(GalliumParser::FnAttributeContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitFnArgumentList(GalliumParser::FnArgumentListContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitSingleFnArgument(GalliumParser::SingleFnArgumentContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitClassDeclaration(GalliumParser::ClassDeclarationContext* ctx) final {
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

    antlrcpp::Any visitBlockStatement(GalliumParser::BlockStatementContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitType(GalliumParser::TypeContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitTypeWithoutRef(GalliumParser::TypeWithoutRefContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitBuiltinType(GalliumParser::BuiltinTypeContext* ctx) final {
      return visitChildren(ctx);
    }

    antlrcpp::Any visitGenericTypeList(GalliumParser::GenericTypeListContext* ctx) final {
      return visitChildren(ctx);
    }
  };
} // namespace

namespace gal {
  std::optional<ast::Program> parse(std::string_view text) noexcept {
    //

    return std::nullopt;
  }
} // namespace gal