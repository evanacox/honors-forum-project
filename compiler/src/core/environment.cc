//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "environment.h"
#include "../ast/program.h"

namespace ast = gal::ast;

namespace {
  class BuildGlobalSymbolTable final : public ast::DeclarationVisitor<void> {
  public:
    BuildGlobalSymbolTable(ast::Program* program, gal::DiagnosticReporter* diagnostics) noexcept
        : diagnostics_{diagnostics} {
      for (auto& decl : program->decls_mut()) {
        decl->accept(this);
      }
    }

    void visit(ast::ImportDeclaration*) final {
      // TODO
    }

    void visit(ast::ImportFromDeclaration*) final {
      // TODO
    }

    void visit(ast::FnDeclaration* declaration) final {
      insert(declaration->proto().name(), gal::Overload(declaration));
    }

    void visit(ast::StructDeclaration* declaration) final {
      insert(declaration->name(), declaration);
    }

    void visit(ast::ClassDeclaration*) final {
      // TODO
    }

    void visit(ast::TypeDeclaration* declaration) final {
      insert(declaration->name(), declaration);
    }

    void visit(ast::MethodDeclaration*) final {
      // TODO
    }

    void visit(ast::ExternalFnDeclaration* declaration) final {
      insert(declaration->proto().name(), gal::Overload(declaration));
    }

    void visit(ast::ExternalDeclaration* declaration) final {
      for (auto& fn : declaration->externals_mut()) {
        fn->accept(this);
      }
    }

    void visit(ast::ConstantDeclaration* declaration) final {
      insert(declaration->name(), declaration);
    }

  private:
    friend class gal::GlobalEnvironment;

    void insert(std::string_view name, ast::Declaration* decl) noexcept {
      auto [it, inserted] = entities_.emplace(name, gal::GlobalEntity{name, decl});

      if (!inserted) {
        auto new_decl = gal::point_out_part(*decl, gal::DiagnosticType::error, "re-declaration was here");
        auto old = gal::point_out_part(it->second.decl(), gal::DiagnosticType::note, "previous declaration was here");
        auto underline = gal::point_out_list(std::move(new_decl), std::move(old));

        diagnostics_->report_emplace(6, gal::into_list(std::move(underline)));
      }
    }

    void insert(std::string_view name, gal::Overload overload) noexcept {
      auto [it, _] = overloads_.emplace(name, gal::OverloadSet{name});
      auto args = overload.proto().args();

      // if any other overloads have the same arguments, overloading
      // is kind of broken. if we try to make it work based on return type
      // or whatever it's just horrible to try to reason about
      for (auto other : it->second.fns()) {
        auto other_args = other.proto().args();

        if (std::equal(args.begin(), args.end(), other_args.begin(), other_args.end())) {
          auto a = gal::point_out_part(other.loc(), gal::DiagnosticType::note, "original overload is here");
          auto b = gal::point_out_part(overload.loc(), gal::DiagnosticType::error, "conflicting overload is here");
          auto error = gal::point_out_list(std::move(a), std::move(b));

          diagnostics_->report_emplace(9, gal::into_list(std::move(error)));

          return;
        }
      }

      it->second.add_overload(overload);
    }

    absl::flat_hash_map<std::string, gal::GlobalEntity> entities_;
    absl::flat_hash_map<std::string, gal::OverloadSet> overloads_;
    gal::DiagnosticReporter* diagnostics_;
  };
} // namespace

namespace gal {
  void OverloadSet::add_overload(Overload overload) noexcept {
#ifndef NDEBUG
    auto args = overload.proto().args();

    assert(std::none_of(functions_.begin(), functions_.end(), [args](auto overload) {
      auto other_args = overload.proto().args();

      return std::equal(args.begin(), args.end(), other_args.begin(), other_args.end());
    }));
#endif

    functions_.push_back(overload);
  }

  GlobalEnvironment::GlobalEnvironment(ast::Program* program, gal::DiagnosticReporter* diagnostics) noexcept {
    BuildGlobalSymbolTable builder(program, diagnostics);

    entities_ = std::move(builder.entities_);
    overloads_ = std::move(builder.overloads_);
  }

  std::optional<GlobalEntity*> GlobalEnvironment::entity(std::string_view name) noexcept {
    auto it = entities_.find(name);

    return (it != entities_.end()) ? std::make_optional(&it->second) : std::nullopt;
  }

  std::optional<const GlobalEntity*> GlobalEnvironment::entity(std::string_view name) const noexcept {
    auto it = entities_.find(name);

    return (it != entities_.end()) ? std::make_optional(&it->second) : std::nullopt;
  }

  std::optional<OverloadSet*> GlobalEnvironment::overloads(std::string_view name) noexcept {
    auto it = overloads_.find(name);

    return (it != overloads_.end()) ? std::make_optional(&it->second) : std::nullopt;
  }

  std::optional<const OverloadSet*> GlobalEnvironment::overloads(std::string_view name) const noexcept {
    auto it = overloads_.find(name);

    return (it != overloads_.end()) ? std::make_optional(&it->second) : std::nullopt;
  }

  bool GlobalEnvironment::contains_any(std::string_view name) const noexcept {
    return overloads_.contains(name) || entities_.contains(name);
  }

  std::optional<const ast::Type*> Scope::get(std::string_view name) const noexcept {
    auto it = variables_.find(name);

    return (it != variables_.end()) ? std::make_optional(&it->second.type()) : std::nullopt;
  }

  bool Scope::add(std::string_view name, ScopeEntity data, gal::DiagnosticReporter* diagnostics) noexcept {
    // while its *probably* fine to use-after-move for `one`, may as well not risk it's not
    // a for-sure safe bet to assume that if `inserted` is false that `data` wasn't actually move-constructed
    // somewhere inside the chain of `flat_hash_map`. thus, we copy it just in case
    auto loc = data.loc();
    auto [it, inserted] = variables_.emplace(std::string{name}, std::move(data));

    if (!inserted) {
      auto a = gal::point_out_part(loc, gal::DiagnosticType::error, "second binding was here");
      auto b = gal::point_out_part(it->second.loc(), gal::DiagnosticType::note, "first binding was here");
      auto c = gal::point_out_list(std::move(a), std::move(b));

      diagnostics->report_emplace(8, gal::into_list(std::move(c)));
    }

    return inserted;
  }

  bool Scope::contains(std::string_view name) const noexcept {
    return variables_.contains(name);
  }

  std::optional<const ast::Type*> Environment::get(std::string_view name) const noexcept {
    for (auto& scope : gal::reverse(scopes_)) {
      if (auto type = scope.get(name)) {
        return type;
      }
    }

    return std::nullopt;
  }

  bool Environment::contains(std::string_view name) const noexcept {
    return std::any_of(scopes_.begin(), scopes_.end(), [name](const gal::Scope& scope) {
      return scope.contains(name);
    });
  }

  void Environment::add(std::string_view name, ScopeEntity data) noexcept {
    assert(!scopes_.empty());

    scopes_.back().add(std::string{name}, std::move(data), diagnostics_);
  }

  void Environment::push(Scope scope) noexcept {
    scopes_.push_back(std::move(scope));
  }

  void Environment::pop() noexcept {
    scopes_.pop_back();
  }

  void Environment::enter_scope() noexcept {
    push(Scope{});
  }

  void Environment::leave_scope() noexcept {
    pop();
  }

  absl::Span<const Scope> Environment::scopes() const noexcept {
    return scopes_;
  }

  Environment::Environment(gal::DiagnosticReporter* reporter) noexcept : diagnostics_{reporter} {}
} // namespace gal