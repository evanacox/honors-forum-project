---
title: "Name Mangling"
permalink: /articles/design/mangling/
excerpt: "How the Gallium compiler handles name mangling."
usemathjax: true
classes: wide
---

Name mangling is the process of "mangling" the generated symbol names in the resulting assembly/object code that the
compiler produces, taking a combination of different factors (e.g module path, name, argument types, etc) and combining
all of them into a true globally unique "name" for the entity.

## Why It's Necessary

All the code that gets linked into an executable ends up with a large amount of overlap. Say you link in the C runtime
library, and you try to combine it with the following Gallium code:

~~~
fn exit() -> void {
  // ...
}

fn main() -> void {
  // ...
}
~~~

The C runtime library will try to invoke `main`, and will probably break everything due to the initialization it does /
how it calls `main`. In addition, you will almost certainly end up with duplicate symbols for `exit`, because C
defines `exit`.

The other issue is with function overloading. How do we put two symbols in the same binary with the same name? If we
just blindly output the names the user gave us, we'd end up with two `to_string` symbols, which is a hard error at link
time.

~~~
fn to_string(x: f64) -> String { 
  ...
}

fn to_string(x: usize) -> String {
  ...
}
~~~

In order to solve this, we need to somehow make our symbol names *unique*, name mangling is the process that
accomplishes this.

## Gallium's Name Mangling

Gallium's name mangling is a run-length encoding heavily inspired by
the [Itanium C++ ABI's name mangling rules](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling).

All patterns begin with `_G`, the "Gallium-reserved" symbol prefix. The C standard
(along with most platforms) require that all symbols starting (or containing) `__` (double underscores) or any symbols
beginning with `_` and a capital letter are reserved for use by the "implementation," aka the platform's C library and
any runtime libraries. In the real world, we can get away with using these symbols as long as we're consistent with
"namespacing" them to ensure uniqueness.

In this case, `_G` is our way of "namespacing" all Gallium symbols from the rest of the world. The Gallium runtime
library also uses functions starting with `__gallium_` for various operations, e.g `__gallium_alloca`
or `__gallium_panic`.

Other ABIs may reserve other prefixes, e.g the Itanium C++ ABI reserves `_Z`, Rust has active proposals for
standardizing name mangling that reserve `_R`, D reserves `_D,` Ada reserves `_ada_`, etc. As long as ours is unique (or
at least unique in the ABIs that end up linked into a Gallium executable), we *should* be fine.

### Examples

Here are some examples of mangled symbol names, and the entity that they map to:

| Entity Signature                                                               | Mangled Name                                  |
| ------------------------------------------------------------------------------ | --------------------------------------------- |
| `fn ::foo(i32, i64) -> void`                                                   | `_GF3fooNlmEv`                                |
| `fn ::square(isize, isize) throws -> isize`                                    | `_GF6squareTooEo`                             |
| `fn ::read_file(&::core::fs::Path) throws -> ::core::String`                   | `_GF9read_fileTR4core2fsU4PathE4coreU6String` |
| `fn ::core::mem::copy(*const byte, *mut byte) -> void`                         | `_G4core3memF4copyNPaQaEv`                    |
| `fn ::__arch::__amd64::__save_fpu_state() -> void`                             | `_G6__arch7__amd64F16__save_fpu_stateNEv`     |
| `const ::core::math::pi: f64`                                                  | `_G4core4mathC2piq`                           |
| `const ::n_threads: usize`                                                     | `_GC9n_threadsi`                              |
| `fn ::whatever(&::long::Name, &::LongType, ::long::Name) throws -> ::LongType` | `_GF8whateverTR4longU4NameRU8LongTypeZ0_EZ1_` |

### Pattern

*MangledName* := `_G` *ModulePrefix* (*FnPattern* | *ConstantPattern*)

### Module Prefix

For a given module `::<a>::<b>::<c>`, it would be mangled as:

`<length in chars of a><a><len of b><b><len of c><c>`

Consider `::core::collections::internal`: `4core11collections8internal`

Or, consider `::__builtin::__simd::__neon`: `9__builtin6__simd6__neon`

Finally, note that the prefix for `::` is simply no prefix at all.

*ModulePrefix* := (\<decimal length\> \<module part name\>)*

### Type Patterns

Builtin types mangle to one or two characters. Types that are the same size still mangle to different symbols, due to
the fact that they can be overloaded on. User-defined types are identified by their names.

| Type                  | Mangled Name                                                                    |
| --------------------- | ------------------------------------------------------------------------------- |
| `void`                | `v`                                                                             |
| `byte`                | `a`                                                                             |
| `bool`                | `b`                                                                             |
| `char`                | `c`                                                                             |
| `u8`                  | `d`                                                                             |
| `u16`                 | `e`                                                                             |
| `u32`                 | `f`                                                                             |
| `u64`                 | `g`                                                                             |
| `u128`                | `h`                                                                             |
| `usize`               | `i`                                                                             |
| `i8`                  | `j`                                                                             |
| `i16`                 | `k`                                                                             |
| `i32`                 | `l`                                                                             |
| `i64`                 | `m`                                                                             |
| `i128`                | `n`                                                                             |
| `isize`               | `o`                                                                             |
| `f32`                 | `p`                                                                             |
| `f64`                 | `q`                                                                             |
| `f128`                | `r`                                                                             |
| `*const T`            | `P` \<mangled name of `T`\>                                                     |
| `*mut T`              | `Q` \<mangled name of `T`\>                                                     |
| `&T`                  | `R` \<mangled name of `T`\>                                                     |
| `&mut T`              | `S` \<mangled name of `T`\>                                                     |
| `[T; N]`              | `A` \<mangled name of `T`\> \<`N`\> `_`                                         |
| `[T]`                 | `B` \<mangled name of `T`\>                                                     |
| `[mut T]`             | `C` \<mangled name of `T`\>                                                     |
| `fn (Args...) -> T`   | `F` (`T` \| `N`) \<mangled name of each in `Args`\> `E` \<mangled name of `T`\> |
| User-Defined Type `T` | *ModulePrefix* `U` \<decimal length of name\> \<name of `T`\>                   |
| Dynamic Interface `T` | *ModulePrefix* `D` \<decimal length of name\> \<name of `T`\>                   |

## Functions

*FnPattern* :=  `F` \<decimal length of name\> \<name\> (`T` | `N`) (\<mangled argument type\>)* 'E' \<mangled return
type\>

Functions encode whether they are throwing (`T`) or non-throwing (`N`) at the ABI level.

The following functions are special-cased for mangling:

| Function Signature   | Mangled Name          |
| -------------------- | --------------------- |
| `fn ::main() -> i32` | `__gallium_user_main` |

## Constants

*ConstantPattern* := `C` \<decimal length of name\> \<name\> \<mangled type\>

Constants also encode slightly more information than necessary at the ABI level, just for safety purposes.

## Substitutions

Substitutions exist for the sake of shortening symbol names that would otherwise be obnoxiously long for no reason.

Every time a user-defined type is encountered by the name demangler, it is put into a "substitution table," with the "
key" being the position of the user-defined type starting at 0 (i.e first instance is 0, second is 1, ...).

Whenever `Z` is encountered, the number following the `Z` is treated is looked up in the table, and the substution is
replaced with the type found in the table.

Ex: `_GF1fN4some4util3libS4VecZ0_v`

- `4some4util3libS4Vec` is encountered, put in table at position `0`
- `Z0_` is encountered, table looks up `0`
- table has `0`, `Z0_` interpreted the same as `4some4util3libS4Vec`

*Substitution* := `Z` \<decimal number\> `_`
