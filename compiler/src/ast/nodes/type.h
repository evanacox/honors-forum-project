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

#include "./type_visitor.h"
#include <memory>

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
    user_defined,
    fn_pointer,
    dyn_interface,
  };

  /// Abstract base type for all "Type" AST nodes
  ///
  /// Is able to be visited by a `TypeVisitorBase`, and can be queried
  /// on whether it's exported and what real type of Type it is
  class Type {
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
    virtual void accept(const ConstTypeVisitorBase& visitor) const = 0;

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
    template <typename T> T accept(const ConstTypeVisitor<T>& visitor) {
      accept(static_cast<const ConstTypeVisitorBase&>(visitor));

      return visitor->move_result();
    }

    /// Virtual dtor so this can be `delete`ed by a `unique_ptr` or whatever
    virtual ~Type() = default;

  protected:
    /// Initializes the state of the Type base class
    ///
    /// \param exported Whether or not this particular Type is marked `export`
    explicit Type(TypeType real) noexcept : real_{real} {}

    /// Protected so only derived can copy
    Type(const Type&) = default;

    /// Protected so only derived can move
    Type(Type&&) = default;

  private:
    TypeType real_;
  };

  /// Represents a "reference" type, i.e `&T` or `&mut T`
  class ReferenceType : public Type {
  public:
    /// Initializes a reference type
    ///
    /// \param mut Whether or not the reference has `mut` in it
    /// \param referenced The type actually being referred to, i.e `T` in `&T`
    ReferenceType(bool mut, std::unique_ptr<Type> referenced) noexcept
        : Type(TypeType::reference),
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
    std::unique_ptr<Type> exchange_type(std::unique_ptr<Type> new_type) noexcept {
      return std::exchange(referenced_, std::move(new_type));
    }

  private:
    bool mutable_;
    std::unique_ptr<Type> referenced_;
  };

  /// Represents a slice type, i.e `[T]`
  class SliceType : public Type {};

  /// Represents a pointer type, i.e `*mut T` or `*const T`
  class PointerType : public Type {};

  /// Represents a builtin integer type, i.e `isize` or `u8` or `i32`
  class BuiltinIntegralType : public Type {};

  /// Represents a builtin floating-point type, i.e `f64` or `f32`
  class BuiltinFloatType : public Type {};

  /// Represents the builtin `bool` type
  class BuiltinBoolType : public Type {};

  /// Represents the builtin `byte` type
  class BuiltinByteType : public Type {};

  /// Represents a **reference** to a user-defined type.
  class UserDefinedType : public Type {};

  /// Represents a function pointer type, i.e `fn (i32) -> i32`
  class FnPointerType : public Type {};

  /// Represents a `dyn` trait type, such as `dyn Addable`
  class DynInterfaceType : public Type {};
} // namespace gal::ast
