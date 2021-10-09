//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021 Evan Cox <evanacox00@gmail.com>. All rights reserved.      //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#pragma once

#include "./declaration_visitor.h"
#include "./modular_id.h"
#include "./statements.h"
#include "./type.h"
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace gal::ast {
  enum class DeclType {
    import_decl,
    import_from_decl,
    fn_decl,
    struct_decl,
    class_decl,
    type_decl,
  };

  /// Abstract base type for all "declaration" AST nodes
  ///
  /// Is able to be visited by a `DeclarationVisitor`, and can be queried
  /// on whether it's exported and what real type of declaration it is
  class Declaration {
  public:
    Declaration() = delete;

    ///
    ///
    /// \return
    [[nodiscard]] bool exported() const noexcept {
      return exported_;
    }

    /// Gets the real declaration type that a declaration actually is
    ///
    /// \return
    [[nodiscard]] DeclType type() const noexcept {
      return real_;
    }

    ///
    ///
    /// \param visitor
    virtual void accept(DeclarationVisitor* visitor) = 0;

    ///
    virtual ~Declaration() = default;

  protected:
    /// Initializes the state of the declaration base class
    ///
    /// \param exported Whether or not this particular declaration is marked `export`
    explicit Declaration(bool exported, DeclType real) noexcept : exported_{exported}, real_{real} {}

    Declaration(const Declaration&) = default;

    Declaration(Declaration&&) = default;

  private:
    bool exported_;
    DeclType real_;
  };

  class ImportDeclaration : public Declaration {
  public:
    explicit ImportDeclaration(bool exported, ModuleID module_imported) noexcept
        : Declaration(exported, DeclType::import_decl),
          mod_{std::move(module_imported)} {}

    [[nodiscard]] const ModuleID& mod() const noexcept {
      return mod_;
    }

    void accept(DeclarationVisitor* visitor) final {
      visitor->visit(this);
    }

  private:
    ModuleID mod_;
  };

  class ImportFromDeclaration final : public Declaration {
  public:
    explicit ImportFromDeclaration(bool exported, std::vector<ModularID> entities) noexcept
        : Declaration(exported, DeclType::import_from_decl),
          entities_{std::move(entities)} {}

    [[nodiscard]] absl::Span<const ModularID> imported_entities() const noexcept {
      return entities_;
    }

    void accept(DeclarationVisitor* visitor) final {
      visitor->visit(this);
    }

  private:
    std::vector<ModularID> entities_;
  };

  class FnDeclaration final : public Declaration {
  public:
    enum class FnAttribute {
      pure,
      throws,
    };

    using Argument = std::pair<std::string, std::unique_ptr<Type>>;

    explicit FnDeclaration(bool exported,
        std::string name,
        std::vector<Argument> args,
        std::vector<FnAttribute> attributes,
        std::unique_ptr<BlockStatement> body) noexcept
        : Declaration(exported, DeclType::fn_decl),
          name_{std::move(name)},
          args_{std::move(args)},
          attributes_{std::move(attributes)},
          body_{std::move(body)} {}

    [[nodiscard]] std::string_view name() const noexcept {
      return name_;
    }

    [[nodiscard]] absl::Span<const Argument> args() const noexcept {
      return args_;
    }

    [[nodiscard]] absl::Span<const FnAttribute> attributes() const noexcept {
      return attributes_;
    }

    [[nodiscard]] BlockStatement* body() noexcept {
      return body_.get();
    }

    void accept(DeclarationVisitor* visitor) final {
      visitor->visit(this);
    }

  private:
    std::string name_;
    std::vector<Argument> args_;
    std::vector<FnAttribute> attributes_;
    std::unique_ptr<BlockStatement> body_;
  };
} // namespace gal::ast