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

#include "absl/types/span.h"
#include <cassert>
#include <charconv>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <variant>

#if defined(__GNUC__) || defined(__clang__)
#define GALLIUM_COLD [[gnu::cold]]
#define GALLIUM_LIKELY(exp) __builtin_expect((exp), true)
#define GALLIUM_UNLIKELY(exp) __builtin_expect((exp), false)
#else
#define GALLIUM_COLD
#endif

namespace gal {
  /// Casts a unique pointer of `U` into a unique pointer of `T`.
  ///
  /// \note Does debug checking with `dynamic_cast` when `NDEBUG` is not defined
  /// \param ptr The pointer to cast
  /// \return The casted pointer
  template <typename T, typename U> std::unique_ptr<T> static_unique_cast(std::unique_ptr<U> ptr) noexcept {
    auto* real = ptr.release();

    assert(dynamic_cast<T*>(real) != nullptr);

    return std::unique_ptr<T>(static_cast<T*>(real));
  }

  /// Equality functor that dereferences its arguments and performs `==` on them.
  ///
  /// \tparam T The type to compare
  template <typename T = void> struct DerefEq {
    constexpr bool operator()(const T& lhs, const T& rhs) const {
      return *lhs == *rhs;
    }
  };

  template <> struct DerefEq<void> {
    template <typename T, typename U> constexpr bool operator()(const T& lhs, const U& rhs) const {
      return *lhs == *rhs;
    }
  };

  /// Takes two optionals and compares for deep equality. If they both have a value,
  /// the optionals are unwrapped and compared using a functor. Otherwise, their has-value states
  /// are compared.
  ///
  /// \param lhs The first optional to compare
  /// \param rhs The second optional to compare
  /// \return `(lhs && rhs) ? (cmp(*lhs, *rhs)) : (lhs.has_value() == rhs.has_value())`
  template <typename T, typename U, typename Cmp = std::equal_to<T>>
  [[nodiscard]] bool unwrapping_equal(std::optional<T> lhs, std::optional<U> rhs, const Cmp& cmp = Cmp{}) noexcept {
    if (lhs.has_value() && rhs.has_value()) {
      return cmp(*lhs, *rhs);
    }

    return lhs.has_value() == rhs.has_value();
  }

  /// Clones a span over cloneable AST nodes and returns a new vector with
  /// the cloned nodes
  ///
  /// \param array The span to clone
  /// \return A new array containing each cloned element
  template <typename T, typename U = T>
  std::vector<std::unique_ptr<U>> clone_span(absl::Span<const std::unique_ptr<T>> array) noexcept {
    auto new_array = std::vector<std::unique_ptr<U>>{};

    for (auto& object : array) {
      static_assert(std::is_same_v<T, U> || std::is_base_of_v<U, T>);

      new_array.emplace_back(static_cast<U*>(object->clone().release()));
    }

    return new_array;
  }

  /// Equivalent to `some.map(|val| val->clone())`
  ///
  /// \param maybe The maybe to map
  /// \return The mapped maybe
  template <typename T>
  std::optional<std::unique_ptr<T>> clone_if(const std::optional<std::unique_ptr<T>>& maybe) noexcept {
    if (maybe.has_value()) {
      return gal::static_unique_cast<T>((*maybe)->clone());
    }

    return std::nullopt;
  }

  /// Checks if a narrowing conversion from `value` to `T` is safe. If it is,
  /// the conversion is performed. Otherwise, `nullopt` is returned
  ///
  /// \tparam T The new type
  /// \tparam U The old type
  /// \param value The value to convert
  /// \return A converted value, if the conversion is safe
  template <typename T, typename U> [[nodiscard]] constexpr std::optional<T> try_narrow(U value) noexcept {
    static_assert(std::numeric_limits<T>::is_signed == std::numeric_limits<U>::is_signed);
    static_assert(std::numeric_limits<T>::digits <= std::numeric_limits<U>::digits);

    if (value <= static_cast<U>(std::numeric_limits<T>::max())
        && value >= static_cast<U>(std::numeric_limits<T>::min())) {
      return static_cast<T>(value);
    }

    return std::nullopt;
  }

  /// Integer exponentiation function for simple exponentiation with integral types
  ///
  /// \tparam T The type of integer to calculate the pow result of
  /// \param base The number to raise to the `exp`th power
  /// \param exp The exponent to raise `base` to
  /// \return The result of `(base)**(exp)`
  template <typename T> [[nodiscard]] constexpr T ipow(T base, T exp) noexcept {
    T result = 1;

    // exponentiation by squaring, decent method of doing it for an unknown `exp`
    while (true) {
      if (exp % 2 == 1) {
        result *= base;
      }

      exp >>= 1;

      if (exp == 0) {
        break;
      }

      base *= base;
    }

    return result;
  }

  /// Uniform method of parsing digits into an integer for the whole compiler,
  /// makes it harder to accidentally use a slow method in some places and not others
  ///
  /// \param text The text to parse
  /// \param base The base to try to parse it in
  /// \return The parsed integer, or an error
  [[nodiscard]] std::variant<std::uint64_t, std::error_code> from_digits(std::string_view text, int base = 10) noexcept;

  /// Uniform method of parsing text into a float for the whole compiler,
  /// makes it harder to accidentally use a slow method in some places and not others
  ///
  /// \param text The text to parse
  /// \param format The format to parse in
  /// \return The parsed float, or an error
  [[nodiscard]] std::variant<double, std::error_code> from_digits(std::string_view text,
      std::chars_format format) noexcept;

  /// Uniform method of converting integers into text for the whole compiler,
  /// makes it harder to accidentally use a slow method in some places and not others
  ///
  /// \param n The number to convert
  /// \param base The base to convert into
  /// \return A string representing the number
  [[nodiscard]] std::string to_digits(std::uint64_t n, int base = 10) noexcept;

  /// Uniform method of converting integers into text for the whole compiler,
  /// makes it harder to accidentally use a slow method in some places and not others
  ///
  /// \param n The number to convert
  /// \param base The base to convert into
  /// \return A string representing the number
  [[nodiscard]] std::string to_digits(std::int64_t n, int base = 10) noexcept;

  /// Uniform method of converting floats into text for the whole compiler,
  /// makes it harder to accidentally use a slow method in some places and not others
  ///
  /// \param n The number to convert
  /// \param format The format to convert into
  /// \return A string representing the number
  [[nodiscard]] std::string to_digits(double n, std::chars_format format = std::chars_format::general) noexcept;

  /// Uniform method of converting floats into text for the whole compiler,
  /// makes it harder to accidentally use a slow method in some places and not others
  ///
  /// \param n The number to convert
  /// \param format The format to convert into
  /// \return A string representing the number
  [[nodiscard]] std::string to_digits(float n, std::chars_format format = std::chars_format::general) noexcept;

  /// Disambiguating function for `to_digits` on any other integral type
  ///
  /// \param n The integer to string-ify
  /// \param base The base to stringify it with
  /// \return The number as a string
  template <typename T> [[nodiscard]] std::string to_digits(T n, int base = 10) noexcept {
    if constexpr (std::is_unsigned_v<T>) {
      return to_digits(static_cast<std::uint64_t>(n), base);
    } else {
      return to_digits(static_cast<std::int64_t>(n), base);
    }
  }

  namespace internal {
    template <typename First, typename...> struct FirstOf { using type = First; };
  } // namespace internal

  /// Turns a set of pointers to diagnostic parts into a vector, since `initializer_list` can't
  /// deal with non-copyable types
  ///
  /// \param args The pointers to make into a list
  /// \return The list
  template <typename... Args, typename T = typename internal::FirstOf<Args...>::type>
  [[nodiscard]] std::vector<T> into_list(Args&&... args) noexcept {
    auto vec = std::vector<T>{};

    (vec.push_back(std::move(args)), ...);

    return vec;
  }

  namespace internal {
    /// Cast that does correctness checking in debug mode
    ///
    /// \param entity The entity to cast
    /// \return The cast result
    template <typename T, typename U, typename = std::enable_if_t<!std::is_pointer_v<T> && !std::is_pointer_v<U>>>
    [[nodiscard]] constexpr T& debug_cast(U& entity) noexcept {
      assert(dynamic_cast<std::remove_reference_t<T>*>(&entity) != nullptr);

      return static_cast<T&>(entity);
    }

    /// Cast that does correctness checking in debug mode
    ///
    /// \param entity The entity to cast
    /// \return The cast result
    template <typename T, typename U> [[nodiscard]] constexpr T debug_cast(U entity) noexcept {
      assert(dynamic_cast<std::remove_pointer_t<T>*>(entity) != nullptr);

      return static_cast<T>(entity);
    }

    template <typename T> class ReverseWrapper {
    public:
      explicit ReverseWrapper(T& i) noexcept : iterable_{i} {}

      ReverseWrapper(const ReverseWrapper&) = default;

      ReverseWrapper(ReverseWrapper&&) = default;

      auto begin() noexcept {
        return std::rbegin(iterable_);
      }

      auto end() noexcept {
        return std::rend(iterable_);
      }

    private:
      T& iterable_;
    };
  } // namespace internal

  /// Effectively a C++17-compatible "reverse" adapter
  ///
  /// \param iterable The iterable object to adapt
  /// \return An adapter that can be iterated over in a ranged-for
  template <typename T> internal::ReverseWrapper<T> reverse(T* iterable) {
    return internal::ReverseWrapper{*iterable};
  }

  /// Effectively a C++17-compatible "reverse" adapter
  ///
  /// \param iterable The iterable object to adapt
  /// \return An adapter that can be iterated over in a ranged-for
  template <typename T> internal::ReverseWrapper<const T> reverse(const T& iterable) {
    return internal::ReverseWrapper{iterable};
  }

  /// Identity functor for use in default arguments
  struct Identity {
    /// Returns whatever was passed in
    ///
    /// \param object The object to return
    /// \return `std::forward<...>(object)`
    template <typename T> constexpr T&& operator()(T&& object) const noexcept {
      return std::forward<T>(object);
    }
  };
} // namespace gal
