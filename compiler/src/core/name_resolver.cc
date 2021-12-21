//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./name_resolver.h"
#include "../ast/visitors.h"
#include "../errors/reporter.h"
#include "../utility/misc.h"
#include "../utility/pretty.h"
#include "./environment.h"
#include "absl/container/flat_hash_set.h"

namespace ast = gal::ast;

namespace {
  class UnscopedResolver final : public ast::AnyVisitorBase<void> {
  public:
    explicit UnscopedResolver(gal::NameResolver* resolver, gal::DiagnosticReporter* diagnostics)
        : resolver_{resolver},
          diagnostics_{diagnostics} {}

    void visit(ast::UnqualifiedUserDefinedType* type) final {
      if (auto qualified = resolver_->qualified_for(type->id())) {
        auto id = std::move(qualified->first);

        if (auto actual_type = resolver_->type(id)) {
          replace_self((*actual_type)->clone());

          return;
        }

        if (auto entity = resolver_->entity(id)) {
          auto a = gal::point_out_part(*type, gal::DiagnosticType::error, "usage was here");
          auto b = gal::point_out_part((*entity)->decl(), gal::DiagnosticType::note, "actual entity is here");

          diagnostics_->report_emplace(10, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
        } else if (auto overloads = resolver_->overloads(id)) {
          auto decls = (*overloads)->fns();
          auto a = gal::point_out_part(*type, gal::DiagnosticType::error, "usage was here");
          auto b = gal::point_out_part(decls.front().decl_base(), gal::DiagnosticType::note, "name refers to this fn");

          diagnostics_->report_emplace(10, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
        } else {
          assert(false);
        }

        return;
      }

      auto a = gal::point_out(*type, gal::DiagnosticType::error, "usage was here");
      auto b = gal::single_message(absl::StrCat("the id given was `", type->id().to_string(), "`"));

      diagnostics_->report_emplace(14, gal::into_list(std::move(a), std::move(b)));
    }

    void visit(ast::UnqualifiedDynInterfaceType*) final {
      assert(false);
    }

    void visit(ast::UnqualifiedIdentifierExpression* identifier) final {
      // if it doesn't have any kind of prefix, it may be a local variable,
      // and we just need to make a fully-qualified one
      if (auto prefix = identifier->id().prefix()) {
        if (auto qualified = resolver_->qualified_for(identifier->id())) {
          auto id = std::move(qualified->first);

          if (auto _ = resolver_->constant(id)) {
            replace_self(std::make_unique<ast::IdentifierExpression>(identifier->loc(), std::move(id)));

            return;
          }

          if (auto _ = resolver_->overloads(id)) {
            replace_self(std::make_unique<ast::IdentifierExpression>(identifier->loc(), std::move(id)));

            return;
          }

          if (auto entity = resolver_->entity(id)) {
            auto a = gal::point_out_part(*identifier, gal::DiagnosticType::error, "usage was here");
            auto b = gal::point_out_part((*entity)->decl(), gal::DiagnosticType::note, "actual entity is here");

            diagnostics_->report_emplace(10, gal::into_list(gal::point_out_list(std::move(a), std::move(b))));
          }
        }

        auto a = gal::point_out(*identifier, gal::DiagnosticType::error, "usage was here");
        auto b = gal::single_message(absl::StrCat("the id given was `", identifier->id().to_string(), "`"));

        diagnostics_->report_emplace(11, gal::into_list(std::move(a), std::move(b)));
      } else {
        replace_self(
            std::make_unique<ast::LocalIdentifierExpression>(identifier->loc(), std::string{identifier->id().name()}));
      }
    }

  private:
    gal::NameResolver* resolver_;
    gal::DiagnosticReporter* diagnostics_;
  };

  template <typename Fn>
  void walk_module_tree(gal::internal::ModuleTable* node, Fn f, std::string_view module = "::") noexcept {
    f(module, &node->env);

    for (auto& [key, value] : node->nested) {
      walk_module_tree(value.get(), f, absl::StrCat(module, "::", key));
    }
  }

  void annotate_type(gal::GlobalEntity* entity, std::string_view module) noexcept {
    auto id = ast::FullyQualifiedID(module, entity->name());

    switch (entity->decl().type()) {
      case ast::DeclType::struct_decl: [[fallthrough]];
      case ast::DeclType::class_decl: {
        *entity->type_owner() =
            std::make_unique<ast::UserDefinedType>(entity->decl().loc(), entity->decl_mut(), std::move(id));
        break;
      }
      case ast::DeclType::type_decl: {
        auto& type_decl = gal::as<gal::ast::TypeDeclaration>(entity->decl());
        *entity->type_owner() = type_decl.aliased().clone();
        break;
      }
      case ast::DeclType::constant_decl: {
        auto* constant = gal::as_mut<ast::ConstantDeclaration>(entity->decl_mut());

        constant->set_id(std::move(id));

        break;
      }
      case ast::DeclType::external_fn_decl: {
        auto* fn = gal::as_mut<ast::ExternalFnDeclaration>(entity->decl_mut());

        fn->set_id(std::move(id));

        break;
      }
      case ast::DeclType::fn_decl: {
        auto* fn = gal::as_mut<ast::ExternalFnDeclaration>(entity->decl_mut());

        fn->set_id(std::move(id));

        break;
      }
      default: break;
    }
  }
} // namespace

namespace gal {
  NameResolver::NameResolver(ast::Program* program, gal::DiagnosticReporter* diagnostics) noexcept
      : program_{program},
        env_{diagnostics},
        root_{{}, GlobalEnvironment(program_, diagnostics)} {
    walk_module_tree(&root_, [this](std::string_view module_name, gal::GlobalEnvironment* env) {
      fully_qualified_.emplace(std::string{module_name}, env);

      for (auto& [name, entity] : env->entities_) {
        annotate_type(&entity, module_name);
      }
    });

    UnscopedResolver(this, diagnostics).walk_ast(program);
  }

  std::optional<const OverloadSet*> NameResolver::overloads(const ast::FullyQualifiedID& id) const noexcept {
    assert(fully_qualified_.contains(id.module_string()));

    return fully_qualified_.at(id.module_string())->overloads(id.name());
  }

  std::optional<const GlobalEntity*> NameResolver::entity(const ast::FullyQualifiedID& id) const noexcept {
    assert(fully_qualified_.contains(id.module_string()));

    return fully_qualified_.at(id.module_string())->entity(id.name());
  }

  std::optional<const ast::Type*> NameResolver::type(const ast::FullyQualifiedID& id) const noexcept {
    using ast::DeclType;

    if (auto entity = this->entity(id)) {
      assert((*entity)->type().has_value());

      return (*entity)->type();
    }

    return std::nullopt;
  }

  std::optional<const ast::StructDeclaration*> NameResolver::struct_type(
      const ast::FullyQualifiedID& id) const noexcept {
    if (auto entity = this->entity(id); entity && (*entity)->decl().is(ast::DeclType::struct_decl)) {
      return &gal::as<ast::StructDeclaration>((*entity)->decl());
    }

    return std::nullopt;
  }

  std::optional<const ast::ConstantDeclaration*> NameResolver::constant(
      const ast::FullyQualifiedID& id) const noexcept {
    if (auto entity = this->entity(id); entity && (*entity)->decl().is(ast::DeclType::constant_decl)) {
      return &gal::as<ast::ConstantDeclaration>((*entity)->decl());
    }

    return std::nullopt;
  }

  void NameResolver::enter_scope() noexcept {
    env_.enter_scope();
  }

  void NameResolver::leave_scope() noexcept {
    env_.leave_scope();
  }

  bool NameResolver::contains_local(std::string_view name) const noexcept {
    return env_.contains(name);
  }

  void NameResolver::add_local(std::string_view name, ScopeEntity data) noexcept {
    env_.add(name, std::move(data));
  }

  std::optional<const ScopeEntity*> NameResolver::local(std::string_view name) const noexcept {
    return env_.get(name);
  }

  std::optional<std::pair<ast::FullyQualifiedID, gal::GlobalEnvironment*>> NameResolver::qualified_for(
      const ast::UnqualifiedID& id) noexcept {
    auto* table = &root_;
    auto s = std::string{"::"};

    // TODO: possibly build a string version of the module name
    // TODO: and use that instead of a tree search?
    // TODO:
    // TODO: current module / imported modules could add "aliases"
    // TODO: and then current module name / the imported module could be
    // TODO: filled in for the alias to com

    if (auto prefix = id.prefix()) {
      auto& mod = **prefix;

      if (!mod.parts().empty()) {
        assert(false);
      }

      // TODO: walk down the tree by each part in `mod.parts()`
      // TODO: modify `table` so it ends up being the right table to check
      assert(mod.from_root());
    } else {
      // TODO: find the actual table that models the current module
      // TODO: right now that's always the root since there's only ever one module

      // TODO: set that current module table to `table`
    }

    if (table->env.contains_any(id.name())) {
      auto full_id = ast::FullyQualifiedID(s, id.name());

      return std::pair{std::move(full_id), &table->env};
    }

    return std::nullopt;
  }
} // namespace gal