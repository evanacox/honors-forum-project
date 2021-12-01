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

#include "../modular_id.h"
#include "../visitors/type_visitor.h"
#include "./ast_node.h"
#include "absl/types/span.h"
#include <memory>
#include <optional>
#include <variant>

namespace gal::ast {
  namespace internal {
    // dirty hack because I'm lazy and don't want to clean up after I moved this fn
    using gal::internal::debug_cast;
  } // namespace internal

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
    user_defined_unqualified,
    user_defined,
    fn_pointer,
    dyn_interface_unqualified,
    dyn_interface,
    error,
    nil_pointer,
    unsized_integer,
    array,
    indirection,
  };

  /// Abstract base type for all "Type" AST nodes
  ///
  /// Is able to be visited by a `TypeVisitorBase`, and can be queried
  /// on whether it's exported and what real type of Type it is
  class Type : public Node {
  public:
    Type() = delete;

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Type() = default;

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
    void accept(TypeVisitorBase* visitor) {
      internal_accept(visitor);
    }

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    void accept(ConstTypeVisitorBase* visitor) const {
      internal_accept(visitor);
    }

    /// Helper that allows a visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(TypeVisitor<T>* visitor) {
      internal_accept(static_cast<TypeVisitorBase*>(visitor));

      return visitor->take_result();
    }

    /// Helper that allows a const visitor to "return" values without needing
    /// dynamic template dispatch.
    ///
    /// \tparam T The type to return
    /// \param visitor The visitor to "return a value from"
    /// \return The value the visitor yielded
    template <typename T> T accept(ConstTypeVisitor<T>* visitor) const {
      internal_accept(static_cast<ConstTypeVisitorBase*>(visitor));

      return visitor->take_result();
    }

    /// Compares two nodes for equality
    ///
    /// \param lhs The first node to compare
    /// \param rhs The second node to compare
    /// \return Whether the two are equal
    [[nodiscard]] friend bool operator==(const Type& lhs, const Type& rhs) noexcept {
      if (lhs.is(TypeType::error) || rhs.is(TypeType::error)) {
        return true;
      }

      return lhs.type() == rhs.type() && lhs.internal_equals(rhs);
    }

    /// Compares two nodes for inequality
    ///
    /// \param lhs The first node to compare
    /// \param rhs The second node to compare
    /// \return Whether the two are unequal
    [[nodiscard]] friend bool operator!=(const Type& lhs, const Type& rhs) noexcept {
      return !(lhs == rhs);
    }

    /// Compares two type nodes for complete equality, including source location.
    /// Equivalent to `a == b && a.loc() == b.loc()`
    ///
    /// \param rhs The other node to compare
    /// \return Whether the nodes are identical in every observable way
    [[nodiscard]] bool fully_equals(const Type& rhs) noexcept {
      return *this == rhs && loc() == rhs.loc();
    }

    /// Clones the node and returns a `unique_ptr` to the copy of the node
    ///
    /// \return A new node with the same observable state
    [[nodiscard]] std::unique_ptr<Type> clone() const noexcept {
      return internal_clone();
    }

    /// Checks if a node is of a particular type in slightly
    /// nicer form than `.type() ==`
    ///
    /// \param type The type to compare against
    /// \return Whether or not the node is of that type
    [[nodiscard]] bool is(TypeType type) const noexcept {
      return real_ == type;
    }

    /// Checks if a node is one of a set of types
    ///
    /// \tparam Args The list of types
    /// \param types The types to check against
    /// \return Whether or not the node is of one of those types
    template <typename... Args> [[nodiscard]] bool is_one_of(Args... types) const noexcept {
      return (is(types) || ...);
    }

  protected:
    /// Initializes the state of the Type base class
    ///
    /// \param exported Whether or not this particular Type is marked `export`
    explicit Type(SourceLoc loc, TypeType real) noexcept : Node(std::move(loc)), real_{real} {}

    /// Protected so only derived can copy
    Type(const Type&) = default;

    /// Protected so only derived can move
    Type(Type&&) = default;

    /// Accepts a visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void internal_accept(TypeVisitorBase* visitor) = 0;

    /// Accepts a const visitor with a `void` return type, and calls the correct
    /// method on that visitor
    ///
    /// \param visitor The visitor to call a method on
    virtual void internal_accept(ConstTypeVisitorBase* visitor) const = 0;

    /// Compares two types for equality
    ///
    /// \note `other` is guaranteed to be the same type
    /// \param other The other node to compare
    /// \return Whether the two types are equal
    [[nodiscard]] virtual bool internal_equals(const Type& other) const noexcept = 0;

    /// Clones the node and returns a `unique_ptr` to the copy of the node
    ///
    /// \return A new node with the same observable state
    [[nodiscard]] virtual std::unique_ptr<Type> internal_clone() const noexcept = 0;

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
    [[nodiscard]] std::unique_ptr<Type>* referenced_owner() noexcept {
      return &referenced_;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

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
    [[nodiscard]] std::unique_ptr<Type>* sliced_owner() noexcept {
      return &sliced_;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

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
    [[nodiscard]] std::unique_ptr<Type>* pointed_owner() noexcept {
      return &pointed_;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    bool mutable_;
    std::unique_ptr<Type> pointed_;
  };

  /// Either in `native_width` state, or the value is positive and a power of 2
  enum class IntegerWidth : std::int64_t { native_width = -1 };

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

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    IntegerWidth size_;
    bool has_sign_;
  };

  /// Gets the integer width of `width`, assumes that `width` is not `native_width` and that
  /// it is actually a positive value
  ///
  /// \param width The width to get the width of
  /// \return The real integer width
  [[nodiscard]] constexpr std::optional<std::int32_t> width_of(IntegerWidth width) noexcept {
    if (width == IntegerWidth::native_width) {
      return std::nullopt;
    } else {
      return static_cast<std::int32_t>(width);
    }
  }

  /// The width of a floating-point type
  enum class FloatWidth {
    ieee_single,
    ieee_double,
    ieee_quadruple,
  };

  /// Represents a builtin floating-point type, i.e `f64` or `f32`
  class BuiltinFloatType final : public Type {
  public:
    /// Creates a built-in type
    ///
    /// \param loc The location of the type in the code
    /// \param size The float width of the type
    explicit BuiltinFloatType(SourceLoc loc, FloatWidth size) noexcept
        : Type(std::move(loc), TypeType::builtin_float),
          size_{size} {}

    /// Gets the integer width of the type
    ///
    /// \return Either a width, or native width
    [[nodiscard]] FloatWidth width() const noexcept {
      return size_;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

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

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;
  };

  /// Represents the builtin `byte` type
  class BuiltinByteType final : public Type {
  public:
    /// Creates a built-in bool type
    ///
    /// \param loc The location of the type in the code
    explicit BuiltinByteType(SourceLoc loc) noexcept : Type(std::move(loc), TypeType::builtin_byte) {}

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;
  };

  class BuiltinCharType final : public Type {
  public:
    /// Creates a built-in bool type
    ///
    /// \param loc The location of the type in the code
    explicit BuiltinCharType(SourceLoc loc) noexcept : Type(std::move(loc), TypeType::builtin_char) {}

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;
  };

  /// Models an unqualified UDT
  class UnqualifiedUserDefinedType final : public Type {
  public:
    /// Creates an unqualified UDT
    ///
    /// \param loc The location in the source code
    /// \param id The identifier
    /// \param generic_params Any generic parameters if they exist
    explicit UnqualifiedUserDefinedType(SourceLoc loc,
        UnqualifiedID id,
        std::vector<std::unique_ptr<Type>> generic_params) noexcept
        : Type(std::move(loc), TypeType::user_defined_unqualified),
          id_{std::move(id)},
          generic_params_{std::move(generic_params)} {}

    /// Gets the actual ID
    ///
    /// \return The ID
    [[nodiscard]] const UnqualifiedID& id() const noexcept {
      return id_;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    UnqualifiedID id_;
    std::vector<std::unique_ptr<Type>> generic_params_;
  };

  /// Represents a **reference** to a user-defined type.
  class UserDefinedType final : public Type {
  public:
    /// Creates a user-defined type
    ///
    /// \param loc The location in the source
    /// \param id The identifier
    /// \param generic_params Any generic parameters
    explicit UserDefinedType(SourceLoc loc,
        FullyQualifiedID id,
        std::vector<std::unique_ptr<Type>> generic_params = {}) noexcept
        : Type(std::move(loc), TypeType::user_defined),
          name_{std::move(id)},
          generic_params_{std::move(generic_params)} {}

    /// Gets the fully qualified ID
    ///
    /// \return The ID
    [[nodiscard]] const FullyQualifiedID& id() const noexcept {
      return name_;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    FullyQualifiedID name_;
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
    [[nodiscard]] std::unique_ptr<Type>* return_type_owner() noexcept {
      return &return_type_;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    std::vector<std::unique_ptr<Type>> args_;
    std::unique_ptr<Type> return_type_;
  };

  /// Represents a `dyn` trait type, such as `dyn Addable`
  class DynInterfaceType : public Type {
  public:
    /// Creates a dynamic interface type
    ///
    /// \param loc Source location
    /// \param id The name of the interface
    /// \param generic_params Any generics provided
    explicit DynInterfaceType(SourceLoc loc,
        FullyQualifiedID id,
        std::vector<std::unique_ptr<Type>> generic_params) noexcept
        : Type(std::move(loc), TypeType::dyn_interface),
          name_{std::move(id)},
          generic_params_{std::move(generic_params)} {}

    /// Gets the fully qualified ID
    ///
    /// \return The ID
    [[nodiscard]] const FullyQualifiedID& id() const noexcept {
      return name_;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    FullyQualifiedID name_;
    std::vector<std::unique_ptr<Type>> generic_params_;
  };

  /// Models an unqualified `dyn foo::Interface<A, B>` type
  class UnqualifiedDynInterfaceType final : public Type {
  public:
    /// Creates an unqualified dyn type
    ///
    /// \param loc The location in the source code
    /// \param id The name of the interface
    /// \param generic_params Any generics provided
    explicit UnqualifiedDynInterfaceType(SourceLoc loc,
        UnqualifiedID id,
        std::vector<std::unique_ptr<Type>> generic_params) noexcept
        : Type(std::move(loc), TypeType::dyn_interface_unqualified),
          id_{std::move(id)},
          generic_params_{std::move(generic_params)} {}

    /// Gets the actual ID
    ///
    /// \return The ID
    [[nodiscard]] const UnqualifiedID& id() const noexcept {
      return id_;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<const std::unique_ptr<Type>>> generic_params() const noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeConstSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the list of generic parameters
    ///
    /// \return The generic parameters
    [[nodiscard]] std::optional<absl::Span<std::unique_ptr<Type>>> generic_params_mut() noexcept {
      if (!generic_params_.empty()) {
        return absl::MakeSpan(generic_params_);
      }

      return std::nullopt;
    }

    /// Gets the owner of the list of generic parameters
    ///
    /// \return The owner of the generic parameters
    [[nodiscard]] std::optional<std::vector<std::unique_ptr<Type>>*> generic_params_owner() noexcept {
      if (!generic_params_.empty()) {
        return &generic_params_;
      }

      return std::nullopt;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    UnqualifiedID id_;
    std::vector<std::unique_ptr<Type>> generic_params_;
  };

  /// Represents a "unit" type for functions that don't have a
  /// meaningful return value. Only has one state, but it is a "type"
  class VoidType final : public Type {
  public:
    /// Creates a built-in "void" type
    ///
    /// \param loc The location of the type in the code
    explicit VoidType(SourceLoc loc) noexcept : Type(std::move(loc), TypeType::builtin_void) {}

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;
  };

  /// Represents the type of a nil literal
  class NilPointerType final : public Type {
  public:
    /// Creates a nil pointer type
    ///
    /// \param loc The location of the literal
    explicit NilPointerType(SourceLoc loc) : Type(std::move(loc), TypeType::nil_pointer) {}

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type&) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;
  };

  /// Represents the type of an integer literal
  class UnsizedIntegerType final : public Type {
  public:
    /// Creates an unsized integer type
    ///
    /// \param loc The location of the literal
    explicit UnsizedIntegerType(SourceLoc loc, std::uint64_t value)
        : Type(std::move(loc), TypeType::unsized_integer),
          value_{value} {}

    [[nodiscard]] std::uint64_t value() const noexcept {
      return value_;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type&) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    std::uint64_t value_;
  };

  /// Models an array type, i.e `[i32; 4]`
  class ArrayType final : public Type {
  public:
    /// Creates an array type
    ///
    /// \param loc The location of the type
    /// \param size The size of the array
    /// \param type The type of each element
    explicit ArrayType(SourceLoc loc, std::uint64_t size, std::unique_ptr<Type> type) noexcept
        : Type(std::move(loc), TypeType::array),
          size_{size},
          type_{std::move(type)} {}

    /// Returns the size of the array
    ///
    /// \return The size of the array
    [[nodiscard]] std::uint64_t size() const noexcept {
      return size_;
    }

    /// Gets the type of the array
    ///
    /// \return The type of the array
    [[nodiscard]] const Type& element_type() const noexcept {
      return *type_;
    }

    /// Gets the type of the array
    ///
    /// \return The type of the array
    [[nodiscard]] Type* element_type_mut() noexcept {
      return type_.get();
    }

    /// Gets the type of the array
    ///
    /// \return The type of the array
    [[nodiscard]] std::unique_ptr<Type>* element_type_owner() noexcept {
      return &type_;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    std::uint64_t size_;
    std::unique_ptr<Type> type_;
  };

  /// Models the magical type produced by `*` that can be assigned to / loaded from
  class IndirectionType final : public Type {
  public:
    explicit IndirectionType(SourceLoc loc, std::unique_ptr<Type> produced) noexcept
        : Type(std::move(loc), TypeType::indirection),
          produced_{std::move(produced)} {}

    /// Gets the type produced by the indirection
    ///
    /// \return The type produced
    [[nodiscard]] const Type& produced() const noexcept {
      return *produced_;
    }

    /// Gets the type produced by the indirection
    ///
    /// \return The type produced
    [[nodiscard]] Type* produced_mut() noexcept {
      return produced_.get();
    }

    /// Gets the type produced by the indirection
    ///
    /// \return The type produced
    [[nodiscard]] std::unique_ptr<Type>* produced_owner() noexcept {
      return &produced_;
    }

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type& other) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;

  private:
    std::unique_ptr<Type> produced_;
  };

  class ErrorType final : public Type {
  public:
    explicit ErrorType() : Type(SourceLoc::nonexistent(), TypeType::error) {}

  protected:
    void internal_accept(TypeVisitorBase* visitor) final;

    void internal_accept(ConstTypeVisitorBase* visitor) const final;

    [[nodiscard]] bool internal_equals(const Type&) const noexcept final;

    [[nodiscard]] std::unique_ptr<Type> internal_clone() const noexcept final;
  };
} // namespace gal::ast
