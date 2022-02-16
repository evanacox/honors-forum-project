//======---------------------------------------------------------------======//
//                                                                           //
// Copyright 2021-2022 Evan Cox <evanacox00@gmail.com>. All rights reserved. //
//                                                                           //
// Use of this source code is governed by a BSD-style license that can be    //
// found in the LICENSE.txt file at the root of this project, or at the      //
// following link: https://opensource.org/licenses/BSD-3-Clause              //
//                                                                           //
//======---------------------------------------------------------------======//

#include "./test_utils.h"
#include "src/core/mangler.h"
#include <gtest/gtest.h>

namespace ast = gal::ast;
using namespace tests;

TEST(mangle_symbols, Main) {
  auto fn = make_fn(make_proto("main"));
  fn->set_id(ast::FullyQualifiedID{"::", "main"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "__gallium_user_main");
  EXPECT_EQ(gal::demangle(mangled), "fn ::main() -> void");
}

TEST(mangle_symbols, Nothing) {
  auto fn = make_fn(make_proto("f"));
  fn->set_id(ast::FullyQualifiedID{"::", "f"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF1fNEv");
  EXPECT_EQ(gal::demangle(mangled), "fn ::f() -> void");
}

TEST(mangle_symbols, WithWeirdSymbols) {
  auto fn = make_fn(make_proto("__copy_avx512",
      gal::into_list(make_arg(ptr(false, byte())), make_arg(ptr(true, byte())), make_arg(integer(false)))));
  fn->set_id(ast::FullyQualifiedID{"::__builtin::__amd64::", "__copy_avx512"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_G9__builtin7__amd64F13__copy_avx512NPaQaiEv");
  EXPECT_EQ(gal::demangle(mangled), "fn ::__builtin::__amd64::__copy_avx512(*const byte, *mut byte, usize) -> void");
}

TEST(mangle_symbols, WithSubstitutions) {
  auto fn = make_fn(make_proto("allocate",
      gal::into_list(make_arg(ref(false, user_defined("::core::mem::", "Layout"))),
          make_arg(ref(true, user_defined("::core::mem::", "Allocation")))),
      user_defined("::core::mem::", "Allocation"),
      gal::into_list(ast::Attribute{ast::AttributeType::builtin_throws, std::vector<std::string>{}})));
  fn->set_id(ast::FullyQualifiedID{"::core::mem::", "allocate"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_G4core3memF8allocateTR4core3memU6LayoutS4core3memU10AllocationEZ1_");
  EXPECT_EQ(gal::demangle(mangled),
      "fn ::core::mem::allocate(&::core::mem::Layout, &mut ::core::mem::Allocation) throws -> ::core::mem::Allocation");
}

TEST(mangle_symbols, WithoutPrefix) {
  auto fn = make_fn(make_proto("allocate",
      gal::into_list(make_arg(ref(false, user_defined("::", "Layout"))), make_arg(user_defined("::", "Allocation"))),
      user_defined("::", "Allocation"),
      gal::into_list(ast::Attribute{ast::AttributeType::builtin_throws, std::vector<std::string>{}})));
  fn->set_id(ast::FullyQualifiedID{"::", "allocate"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF8allocateTRU6LayoutU10AllocationEZ1_");
  EXPECT_EQ(gal::demangle(mangled), "fn ::allocate(&::Layout, ::Allocation) throws -> ::Allocation");
}

TEST(mangle_symbols, DifferentPrefixDifferentSubstitution) {
  auto fn = make_fn(make_proto("do_thing",
      gal::into_list(make_arg(user_defined("::foo::", "Bar")),
          make_arg(user_defined("::quux::", "Bar")),
          make_arg(user_defined("::", "Bar"))),
      user_defined("::", "Bar"),
      gal::into_list(ast::Attribute{ast::AttributeType::builtin_throws, std::vector<std::string>{}})));
  fn->set_id(ast::FullyQualifiedID{"::foo::bar::baz::", "do_thing"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_G3foo3bar3bazF8do_thingT3fooU3Bar4quuxU3BarU3BarEZ2_");
  EXPECT_EQ(gal::demangle(mangled), "fn ::foo::bar::baz::do_thing(::foo::Bar, ::quux::Bar, ::Bar) throws -> ::Bar");
}

TEST(mangle_symbols, MultipleSubstitutions) {
  auto fn = make_fn(make_proto("f",
      gal::into_list(make_arg(user_defined("::s::", "S")),
          make_arg(user_defined("::s::", "S")),
          make_arg(user_defined("::q::", "Q"))),
      user_defined("::q::", "Q")));
  fn->set_id(ast::FullyQualifiedID{"::", "f"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF1fN1sU1SZ0_1qU1QEZ1_");
  EXPECT_EQ(gal::demangle(mangled), "fn ::f(::s::S, ::s::S, ::q::Q) -> ::q::Q");
}

TEST(mangle_symbols, TypeLikeNames) {
  auto fn = make_fn(make_proto("lol",
      gal::into_list(make_arg(user_defined("::", "a")),
          make_arg(user_defined("::", "b")),
          make_arg(user_defined("::", "c"))),
      user_defined("::", "U")));
  fn->set_id(ast::FullyQualifiedID{"::", "lol"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF3lolNU1aU1bU1cEU1U");
  EXPECT_EQ(gal::demangle(mangled), "fn ::lol(::a, ::b, ::c) -> ::U");
}

TEST(mangle_symbols, NestedFnPointers) {
  auto a = fn_ptr(void_type(), integer(true, 8));
  auto b = fn_ptr(std::move(a), float_of(ast::FloatWidth::ieee_quadruple));
  auto c = fn_ptr(integer(true), integer(false), std::move(b));
  auto d = fn_ptr(char_type(), std::move(c));
  auto e = fn_ptr(integer(false, 128), std::move(d));

  auto fn = make_fn(make_proto("the_j", gal::into_list(make_arg(std::move(e)))));
  fn->set_id(ast::FullyQualifiedID{"::", "the_j"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF5the_jNFNFNFNiFNrEFNjEvEoEcEhEv");
  EXPECT_EQ(gal::demangle(mangled),
      "fn ::the_j(fn(fn(fn(usize, fn(f128) -> fn(i8) -> void) -> isize) -> char) -> u128) -> void");
}

TEST(mangle_symbols, NestedSlices) {
  auto fn = make_fn(make_proto("f",
      gal::into_list(make_arg(slice_of(false,
          slice_of(false, slice_of(false, slice_of(false, float_of(ast::FloatWidth::ieee_single)))))))));
  fn->set_id(ast::FullyQualifiedID{"::WIthNUmb3rsF", "f"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_G12WIthNUmb3rsFF1fNBBBBpEv");
  EXPECT_EQ(gal::demangle(mangled), "fn ::WIthNUmb3rsF::f([[[[f32]]]]) -> void");
}

TEST(mangle_symbols, NestedMutSlices) {
  auto fn = make_fn(make_proto("blah",
      gal::into_list(make_arg(
          slice_of(true, slice_of(true, slice_of(true, slice_of(true, float_of(ast::FloatWidth::ieee_double)))))))));
  fn->set_id(ast::FullyQualifiedID{"::blah::blah::blah", "blah"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_G4blah4blah4blahF4blahNCCCCqEv");
  EXPECT_EQ(gal::demangle(mangled), "fn ::blah::blah::blah::blah([mut [mut [mut [mut f64]]]]) -> void");
}

TEST(mangle_symbols, NestedStaticArray) {
  auto fn = make_fn(make_proto("k",
      gal::into_list(make_arg(array_of(12, array_of(6, array_of(3, dyn_user_defined("::core::traits::", "Fn"))))))));
  fn->set_id(ast::FullyQualifiedID{"::", "k"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF1kNAAA4core6traitsD2Fn3_6_12_Ev");
  EXPECT_EQ(gal::demangle(mangled), "fn ::k([[[dyn ::core::traits::Fn; 3]; 6]; 12]) -> void");
}

TEST(mangle_symbols, EveryBuiltinType) {
  auto fn = make_fn(make_proto("abcdefghijklmnopqr",
      gal::into_list(make_arg(byte()),
          make_arg(bool_type()),
          make_arg(char_type()),
          make_arg(integer(false, 8)),
          make_arg(integer(false, 16)),
          make_arg(integer(false, 32)),
          make_arg(integer(false, 64)),
          make_arg(integer(false, 128)),
          make_arg(integer(false)),
          make_arg(integer(true, 8)),
          make_arg(integer(true, 16)),
          make_arg(integer(true, 32)),
          make_arg(integer(true, 64)),
          make_arg(integer(true, 128)),
          make_arg(integer(true)),
          make_arg(float_of(ast::FloatWidth::ieee_single)),
          make_arg(float_of(ast::FloatWidth::ieee_double)),
          make_arg(float_of(ast::FloatWidth::ieee_quadruple)))));

  fn->set_id(ast::FullyQualifiedID{"::", "abcdefghijklmnopqr"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF18abcdefghijklmnopqrNabcdefghijklmnopqrEv");
  EXPECT_EQ(gal::demangle(mangled),
      "fn ::abcdefghijklmnopqr(byte, bool, char, u8, u16, u32, u64, u128, usize, i8, i16, i32, i64, i128, isize, f32, "
      "f64, f128) -> void");
}

TEST(mangle_symbols, PointerType) {
  auto fn = make_fn(make_proto("f",
      gal::into_list(make_arg(ptr(false, byte())),
          make_arg(ptr(true, float_of(ast::FloatWidth::ieee_single))),
          make_arg(ptr(false, user_defined("::", "Hello"))),
          make_arg(ptr(true, ref(false, dyn_user_defined("::", "Goodbye")))),
          make_arg(ptr(false, ptr(true, ptr(false, ptr(true, char_type()))))))));

  fn->set_id(ast::FullyQualifiedID{"::", "f"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF1fNPaQpPU5HelloQRD7GoodbyePQPQcEv");
  EXPECT_EQ(gal::demangle(mangled),
      "fn ::f(*const byte, *mut f32, *const ::Hello, *mut &dyn ::Goodbye, *const *mut *const *mut char) -> void");
}

TEST(mangle_symbols, RefType) {
  auto fn = make_fn(make_proto("f",
      gal::into_list(make_arg(ref(false, byte())),
          make_arg(ref(true, float_of(ast::FloatWidth::ieee_single))),
          make_arg(ref(false, user_defined("::", "Hello"))),
          make_arg(ref(true, ref(false, dyn_user_defined("::", "Goodbye")))),
          make_arg(ref(false, ref(true, ref(false, ref(true, char_type()))))))));

  fn->set_id(ast::FullyQualifiedID{"::", "f"});

  auto mangled = gal::mangle(*fn);
  EXPECT_EQ(mangled, "_GF1fNRaSpRU5HelloSRD7GoodbyeRSRScEv");
  EXPECT_EQ(gal::demangle(mangled), "fn ::f(&byte, &mut f32, &::Hello, &mut &dyn ::Goodbye, &&mut &&mut char) -> void");
}

TEST(mangle_symbols, ConstantPi) {
  auto c = make_const("pi_full_precision", float_of(ast::FloatWidth::ieee_quadruple));
  c->set_id(ast::FullyQualifiedID{"::core::math::internal::", "pi_full_precision"});

  auto mangled = gal::mangle(*c);
  EXPECT_EQ(mangled, "_G4core4math8internalC17pi_full_precisionr");
  EXPECT_EQ(gal::demangle(mangled), "const ::core::math::internal::pi_full_precision: f128");
}

TEST(mangle_symbols, ConstantArray) {
  auto c = make_const("data", array_of(32, byte()));
  c->set_id(ast::FullyQualifiedID{"::main::", "data"});

  auto mangled = gal::mangle(*c);
  EXPECT_EQ(mangled, "_G4mainC4dataAa32_");
  EXPECT_EQ(gal::demangle(mangled), "const ::main::data: [byte; 32]");
}

TEST(mangle_symbols, ConstantString) {
  auto c = make_const("str", slice_of(false, char_type()));
  c->set_id(ast::FullyQualifiedID{"::", "str"});

  auto mangled = gal::mangle(*c);
  EXPECT_EQ(mangled, "_GC3strBc");
  EXPECT_EQ(gal::demangle(mangled), "const ::str: [char]");
}

TEST(mangle_symbols, ConstantWeird) {
  auto c = make_const("weird",
      fn_ptr(ptr(true, fn_ptr(ref(false, dyn_user_defined("::__builtin::", "__Integral")), byte())), integer(false)));
  c->set_id(ast::FullyQualifiedID{"::", "weird"});

  auto mangled = gal::mangle(*c);
  EXPECT_EQ(mangled, "_GC5weirdFNiEQFNaER9__builtinD10__Integral");
  EXPECT_EQ(gal::demangle(mangled), "const ::weird: fn(usize) -> *mut fn(byte) -> &dyn ::__builtin::__Integral");
}