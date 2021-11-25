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
#include "../utility/misc.h"
#include "./environment.h"
#include "absl/container/flat_hash_set.h"

namespace ast = gal::ast;

namespace {
  class UnscopedResolver final : public ast::AnyVisitorBase<void> {
  public:
    explicit UnscopedResolver(gal::NameResolver* resolver) : resolver_{resolver} {}

    void visit(ast::UnqualifiedUserDefinedType* type) final {
      if (auto qualified = resolver_->qualified_for(type->id())) {
        auto& [id, env] = *qualified;

        if (auto actual_type = resolver_->type(id)) {
          replace_self((*actual_type)->clone());
        }
      }
    }

    void visit(ast::UnqualifiedDynInterfaceType*) final {
      assert(false);
    }

    void visit(ast::UnqualifiedIdentifierExpression* identifier) final {
      if (auto prefix = identifier->id().prefix()) {
        //
      }
    }

  private:
    gal::NameResolver* resolver_;
  };

  template <typename Fn>
  void walk_module_tree(gal::internal::ModuleTable* node, Fn f, std::string module = "::") noexcept {
    f(std::string_view{module}, &node->env);

    for (auto& [key, value] : node->nested) {
      walk_module_tree(value.get(), f, absl::StrCat(module, "::", key));
    }
  }

  void annotate_type(gal::GlobalEntity* entity, std::string_view module) noexcept {
    switch (entity->decl().type()) {
      case ast::DeclType::struct_decl: [[fallthrough]];
      case ast::DeclType::class_decl: {
        auto id = ast::FullyQualifiedID(std::string{module}, std::string{entity->name()});
        *entity->type_owner() = std::make_unique<ast::UserDefinedType>(entity->decl().loc(), std::move(id));
        break;
      }
      case ast::DeclType::type_decl: {
        auto& type_decl = gal::internal::debug_cast<const gal::ast::TypeDeclaration&>(entity->decl());
        *entity->type_owner() = type_decl.aliased().clone();
        break;
      }
      default: break;
    }
  }
} // namespace

namespace gal {
  NameResolver::NameResolver(ast::Program* program, std::vector<Diagnostic>* diagnostics) noexcept
      : program_{program},
        root_{{}, GlobalEnvironment(program_, diagnostics)} {
    walk_module_tree(&root_, [this](std::string_view module_name, gal::GlobalEnvironment* env) {
      fully_qualified_.emplace(std::string{module_name}, env);

      for (auto& [name, entity] : env->entities_) {
        annotate_type(&entity, module_name);
      }
    });

    (void)0;
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

  std::optional<const ast::Type*> NameResolver::get_local(std::string_view name) const noexcept {
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
      auto full_id = ast::FullyQualifiedID(std::move(s), std::string{id.name()});

      return std::pair{std::move(full_id), &table->env};
    }

    return std::nullopt;
  }
} // namespace gal