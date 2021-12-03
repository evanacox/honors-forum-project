---
layout: default
title: Name Mangling
nav_order: 4
parent: Compiler Design
---

# Name Mangling
Name mangling is the process of "mangling" the generated symbol names in the
resulting assembly/object code that the compiler produces, taking a combination
of different factors (e.g module path, name, argument types, etc) and combining
all of them into a true globally unique "name" for the entity. 

## Why it's Necessary
All the code that gets linked into an executable ends up with a large amount of overlap. Say you
link in the C runtime library, and you try to combine it with the following Gallium code:

```
fn exit() -> void {
  // ...
}

fn main() -> void {
  // ...
}
```

The C runtime library will try to invoke `main`, and will probably break everything
due to the initialization it does / how it calls `main`. In addition, you will almost certainly
end up with duplicate symbols for `exit`, because C defines `exit`. 

The other issue is with overloading. How do we put two symbols in the same binary with the same name?

```
fn to_string(x: f64) -> String { 
  ...
}

fn to_string(x: usize) -> String {
  ...
}
```

In order to solve this, we need to somehow make our symbol names *unique*, name mangling is the process
that accomplishes this. 

## Gallium's Name Mangling
Gallium's name mangling is a run-length encoding heavily inspired by 
the [Itanium C++ ABI's name mangling rules](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#mangling).

All patterns begin with `_G`, the "Gallium-reserved" symbol prefix. The C standard 
(along with most platforms) require that all symbols starting (or containing) `__` (double
underscores) or any symbols beginning with `_` and a capital letter are reserved for use 
by the "implementation," aka the platform's C library. In the real world, we can get away
with using these symbols as long as we're consistent with "namespacing" them.

In this case, `_G` is our way of "namespacing" all Gallium symbols from the rest of the world. 
The Gallium runtime library also uses functions starting with `__gallium_` for various
operations, e.g `__gallium_alloca` or `__gallium_panic`. 

Other ABIs may reserve other prefixes, e.g the Itanium C++ ABI reserves `_Z`, 
Rust has active proposals for standardizing name mangling that reserve `_R`,
D reserves `_D,` Ada reserves `_ada_`, etc. As long as ours is unique (or at
least unique in the ABIs that end up linked into a Gallium executable), we are fine.

### Pattern
*MangledName* := `_G` *ModulePrefix* (*FnPattern* | *ConstantPattern*) 

### Module Prefix
For a given module `<a>::<b>::<c>`, it would be mangled as:

`M<length in chars of a><a><len of b><b><len of c><c>`

Consider `core::collections::internal`: `M4core11collections8internal`

Or, consider `__builtin::__simd::__neon`: `M9__builtin6__simd6__neon`

*ModulePrefix* := `M` (<decimal length> <module part name>)+

### Type Patterns
Builtin types mangle to one or two characters. Types that are the same 
size still mangle to different symbols, due to the fact that they can 
be overloaded on.

Any user-defined types are simply mangled by 

| Type       | Mangled Name              |
|------------|---------------------------|
| `bool`     | `a`                       |
| `byte`     | `b`                       |
| `char`     | `c`                       |
| `u8`       | `d`                       |
| `u16`      | `e`                       |
| `u32`      | `f`                       |
| `u64`      | `g`                       |
| `usize`    | `h`                       |
| `i8`       | `i`                       |
| `i16`      | `j`                       |
| `i32`      | `k`                       |
| `i64`      | `l`                       |
| `isize`    | `m`                       |
| `*const T` | `P` <mangled name of `T`> |
| `*mut T`   | `Q` <mangled name of `T`> |
| `&T`       | `R` <mangled name of `T`> |
| `&mut T`   | `S` <mangled name of `T`> |

## Functions
*FnPattern* := <name> `F` (<mangled argument type>)+
