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

#include "./modular_id.h"
#include "./node.h"
#include "./type_visitor.h"
#include "absl/types/span.h"
#include <memory>
#include <optional>
#include <variant>

namespace gal::ast {
  enum class TypeType {
    reference,
    slice,
    pointer,
    builtin_integral,
    builtin_float,
    builtin_bool,
    builtin_byte,
    builtin_char,
    builtin_void,
    user_defined,
    fn_pointer,
    dyn_interface,
  };

  /// Abstract base type for all "Type" AST nodes
  ///
  /// Is able to be visited by a `TypeVisitorBase`, and can be queried
  /// on whether it's exported and what real type of Type it is
  class Type : public Node {
  public:
    Type() = delete;

    /// Gets the real Type type that the Type actually is
    ///
    /// \return The type of the Type
    [[nodiscard]] TypeType type() const noexcept {
      return real_;
    }

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void accept(TypeVisitorBase* visitor) = 0;

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void accept(ConstTypeVisitorBase* visitor) const = 0;

    /// Helper that allows a visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(TypeVisitor<T>* visitor) {
      accept(static_cast<TypeVisitorBase*>(visitor));

      return visitor->move_result();
    }

    /// Helper that allows a const visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(ConstTypeVisitor<T>* visitor) const {
      accept(static_cast<ConstTypeVisitorBase*>(visitor));

      return visitor->move_result();
    }

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Type() = default;

  protected:
    /// Initializes the state of the Type base class
    ///
    /// \param exported Whether or not this particular Type is marked `export`
    explicit Type(SourceLoc loc, TypeType real) noexcept : Node(std::move(loc)), real_{real} {}

    /// Protected so only derived can copy
    Type(const Type&) = default;

    /// Protected so only derived can move
    Type(Type&&) = default;

  private:
    TypeType real_;
  };

  /// Represents a "reference" type, i.e `&T` or `&mut T`
  class ReferenceType final : public Type {
  public:
    /// Initializes a reference type
    ///
    /// \param mut Whether or not the reference has `mut` in it
    /// \param referenced The type actually being referred to, i.e `T` in `&T`
    explicit ReferenceType(SourceLoc loc, bool mut, std::unique_ptr<Type> referenced) noexcept
        : Type(std::move(loc), TypeType::reference),
          mutable_{mut},
          referenced_{std::move(referenced)} {}

    /// Checks whether the reference is mutable
    ///
    /// \return Whether or not the reference is mutable
    [[nodiscard]] bool mut() const noexcept {
      return mutable_;
    }

    /// Gets the type being referenced, i.e the `T` in `&mut T`
    ///
    /// \return The type
    [[nodiscard]] const Type& referenced() const noexcept {
      return *referenced_;
    }

    /// Returns a mutable pointer to the referenced type
    ///
    /// \return A mutable pointer
    [[nodiscard]] Type* referenced_mut() noexcept {
      return referenced_.get();
    }

    /// Exchanges the referenced type with another type
    ///
    /// \param new_type The new type to be referencing
    /// \return The old type that was being referenced
    std::unique_ptr<Type> exchange_referenced(std::unique_ptr<Type> new_type) noexcept {
      return std::exchange(referenced_, std::move(new_type));
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    bool mutable_;
    std::unique_ptr<Type> referenced_;
  };

  /// Represents a slice type, i.e `[T]`
  class SliceType final : public Type {
  public:
    /// Creates a Slice type
    ///
    /// \param sliced The type contained in the slice
    explicit SliceType(SourceLoc loc, std::unique_ptr<Type> sliced) noexcept
        : Type(std::move(loc), TypeType::slice),
          sliced_{std::move(sliced)} {}

    /// Gets the type being sliced, i.e the `T` in `&mut T`
    ///
    /// \return The type
    [[nodiscard]] const Type& sliced() const noexcept {
      return *sliced_;
    }

    /// Returns a mutable pointer to the sliced type
    ///
    /// \return A mutable pointer
    [[nodiscard]] Type* sliced_mut() noexcept {
      return sliced_.get();
    }

    /// Exchanges the sliced type with another type
    ///
    /// \param new_type The new type to be referencing
    /// \return The old type that was being sliced
    std::unique_ptr<Type> exchange_sliced(std::unique_ptr<Type> new_type) noexcept {
      return std::exchange(sliced_, std::move(new_type));
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::unique_ptr<Type> sliced_;
  };

  /// Represents a pointer type, i.e `*mut T` or `*const T`
  class PointerType final : public Type {
  public:
    /// Initializes a reference type
    ///
    /// \param mut Whether or not the reference has `mut` in it
    /// \param referenced The type actually being referred to, i.e `T` in `*mut T`
    explicit PointerType(SourceLoc loc, bool mut, std::unique_ptr<Type> pointed_to) noexcept
        : Type(std::move(loc), TypeType::pointer),
          mutable_{mut},
          pointed_{std::move(pointed_to)} {}

    /// Checks whether the reference is mutable
    ///
    /// \return Whether or not the reference is mutable
    [[nodiscard]] bool mut() const noexcept {
      return mutable_;
    }

    /// Gets the type being pointed, i.e the `T` in `&mut T`
    ///
    /// \return The type
    [[nodiscard]] const Type& pointed() const noexcept {
      return *pointed_;
    }

    /// Returns a mutable pointer to the pointed type
    ///
    /// \return A mutable pointer
    [[nodiscard]] Type* pointed_mut() noexcept {
      return pointed_.get();
    }

    /// Exchanges the pointed type with another type
    ///
    /// \param new_type The new type to be referencing
    /// \return The old type that was being pointed
    std::unique_ptr<Type> exchange_pointed(std::unique_ptr<Type> new_type) noexcept {
      return std::exchange(pointed_, std::move(new_type));
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    bool mutable_;
    std::unique_ptr<Type> pointed_;
  };

  /// Either in `native_width` state, or the value is positive and a power of 2
  enum class IntegerWidth : std::int32_t { native_width = -1 };

  /// Represents a builtin integer type, i.e `isize` or `u8` or `i32`
  class BuiltinIntegralType final : public Type {
  public:
    /// Creates a built-in type
    ///
    /// \param loc The location of the type in the code
    /// \param has_sign Whether the type is `signed`
    /// \param size The integer width of the type
    explicit BuiltinIntegralType(SourceLoc loc, bool has_sign, IntegerWidth size) noexcept
        : Type(std::move(loc), TypeType::builtin_integral),
          size_{size},
          has_sign_{has_sign} {}

    /// Checks whether the integer is signed
    ///
    /// \return Whether or not the integer is signed
    [[nodiscard]] bool has_sign() const noexcept {
      return has_sign_;
    }

    /// Gets the integer width of the type
    ///
    /// \return Either a width, or native width
    [[nodiscard]] IntegerWidth width() const noexcept {
      return size_;
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    IntegerWidth size_;
    bool has_sign_;
  };

  /// Gets the integer width of `width`, assumes that `width` is not `native_width` and that
  /// it is actually a positive value
  ///
  /// \param width The width to get the width of
  /// \return The real integer width
  [[nodiscard]] constexpr std::int32_t width_of(IntegerWidth width) noexcept {
    assert(width != IntegerWidth::native_width);
    assert(static_cast<std::int32_t>(width) > 0);

    return static_cast<std::int32_t>(width);
  }

  /// The width of a floating-point type
  enum class FloatWidth {
    ieee_single,
    ieee_double,
    ieee_quadruple,
  };

  /// Represents a builtin floating-point type, i.e `f64` or `f32`
  class BuiltinFloatType final : public Type {
    /// Creates a built-in type
    ///
    /// \param loc The location of the type in the code
    /// \param size The float width of the type
    explicit BuiltinFloatType(SourceLoc loc, IntegerWidth size) noexcept
        : Type(std::move(loc), TypeType::builtin_float),
          size_{size} {}

    /// Gets the integer width of the type
    ///
    /// \return Either a width, or native width
    [[nodiscard]] FloatWidth width() const noexcept {
      return size_;
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    FloatWidth size_;
  };

  /// Represents the builtin `bool` type
  class BuiltinBoolType final : public Type {
  public:
    /// Creates a built-in bool type
    ///
    /// \param loc The location of the type in the code
    explicit BuiltinBoolType(SourceLoc loc) noexcept : Type(std::move(loc), TypeType::builtin_bool) {}

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }
  };

  /// Represents the builtin `byte` type
  class BuiltinByteType final : public Type {
    /// Creates a built-in bool type
    ///
    /// \param loc The location of the type in the code
    explicit BuiltinByteType(SourceLoc loc) noexcept : Type(std::move(loc), TypeType::builtin_byte) {}

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }
  };
  class BuiltinCharType final : public Type {
    /// Creates a built-in bool type
    ///
    /// \param loc The location of the type in the code
    explicit BuiltinCharType(SourceLoc loc) noexcept : Type(std::move(loc), TypeType::builtin_char) {}

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }
  };

  /// Represents a **reference** to a user-defined type.
  class UserDefinedType final : public Type {
  public:
    struct Unqualified {
      std::optional<ModuleID> module_prefix;
      std::string name;
    };

    explicit UserDefinedType(SourceLoc loc,
        std::optional<ModuleID> prefix,
        std::string name,
        std::vector<std::unique_ptr<Type>> generic_params) noexcept
        : Type(std::move(loc), TypeType::user_defined),
          name_{Unqualified{std::move(prefix), std::move(name)}},
          generic_params_{std::move(generic_params)} {}

    [[nodiscard]] bool is_qualified() const noexcept {
      return std::holds_alternative<FullyQualifiedID>(name_);
    }

    [[nodiscard]] const Unqualified& unqualified() const noexcept {
      assert(!is_qualified());

      return std::get<Unqualified>(name_);
    }

    [[nodiscard]] Unqualified* unqualified_mut() noexcept {
      assert(!is_qualified());

      return &std::get<Unqualified>(name_);
    }

    void qualify(FullyQualifiedID id) noexcept {
      assert(!is_qualified());

      name_ = std::move(id);
    }

    [[nodiscard]] absl::Span<const std::unique_ptr<Type>> generic_params() const noexcept {
      return generic_params_;
    }

    [[nodiscard]] absl::Span<std::unique_ptr<Type>> generic_params_mut() noexcept {
      return absl::MakeSpan(generic_params_);
    }

    /// Accepts a visitor that's able to mutate the type
    ///
    /// \param visitor The visitor
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor that's unable to mutate the type
    ///
    /// \param visitor The visitor
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::variant<Unqualified, FullyQualifiedID> name_;
    std::vector<std::unique_ptr<Type>> generic_params_;
  };

  /// Represents a function pointer type, i.e `fn (i32) -> i32`
  class FnPointerType final : public Type {
  public:
    /// Initializes a function pointer type
    ///
    /// \param loc The location in the source
    /// \param args The list of argument types
    /// \param return_type The return type
    explicit FnPointerType(SourceLoc loc,
        std::vector<std::unique_ptr<Type>> args,
        std::unique_ptr<Type> return_type) noexcept
        : Type(std::move(loc), TypeType::fn_pointer),
          args_{std::move(args)},
          return_type_{std::move(return_type)} {}

    /// Gets a span over the arguments
    ///
    /// \return The argument types
    [[nodiscard]] absl::Span<const std::unique_ptr<Type>> args() const noexcept {
      return args_;
    }

    /// Gets a mutable span over the arguments
    ///
    /// \return The argument types
    [[nodiscard]] absl::Span<std::unique_ptr<Type>> args_mut() noexcept {
      return absl::MakeSpan(args_);
    }

    /// Returns the return type of the fn pointer
    ///
    /// \return The function return type
    [[nodiscard]] const Type& return_type() const noexcept {
      return *return_type_;
    }

    /// Mutable pointer to the return type
    ///
    /// \return The return type
    [[nodiscard]] Type* return_type_mut() noexcept {
      return return_type_.get();
    }

    /// Exchanges the return type with a new one, and returns the old
    ///
    /// \param new_type The new type to use
    /// \return The old type
    [[nodiscard]] std::unique_ptr<Type> exchange_return_type(std::unique_ptr<Type> new_type) noexcept {
      return std::exchange(return_type_, std::move(new_type));
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }

  private:
    std::vector<std::unique_ptr<Type>> args_;
    std::unique_ptr<Type> return_type_;
  };

  /// Represents a `dyn` trait type, such as `dyn Addable`
  class DynInterfaceType : public Type {};

  /// Represents a "unit" type for functions that don't have a
  /// meaningful return value. Only has one state, but it is a "type"
  class VoidType final : public Type {
  public:
    /// Creates a built-in "void" type
    ///
    /// \param loc The location of the type in the code
    explicit VoidType(SourceLoc loc) noexcept : Type(std::move(loc), TypeType::builtin_void) {}

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(TypeVisitorBase* visitor) final {
      visitor->visit(this);
    }

    /// Accepts a visitor and calls the right method on it
    ///
    /// \param visitor The visitor to accept
    void accept(ConstTypeVisitorBase* visitor) const final {
      visitor->visit(*this);
    }
  };
} // namespace gal::ast
