---
layout: default
title: Types
nav_order: 1
parent: Gallium Language
---

# Types

Gallium is a strongly-typed and statically-typed language, and takes after C
in that types don't have magic properties just because they're user-defined.

All types are "value types" in that they are automatically copied if you don't pass
them by reference or by pointer. 

## Builtin Types

- `byte`: A byte type, similar to `u8`.
- `char`: A character type, also similar to `u8`
- `u8` / `u16` / `u32` / `u64`: Unsigned integer types that are 8, 16, 32 and 64 bits long, respectively. 
- `i8` / `i16` / `i32` / `i64`: Signed integer types that are 8, 16, 32 and 64 bits long, respectively.
- `usize`/`isize`: Unsigned/signed integer types that are the same size as a pointer (64-bit on 64-bit platforms).
- `bool`: A boolean type

## Array-like Types

- `[T]` / `[mut T]`: Slices, also known as "array views." They are a reference to part of an array
- `[T; N]`: Statically-sized arrays

Slices have the "fields" `.data` and `.size`. 

- `.size` is the length of the slice, it's an `isize`
- `.data` is a pointer to the array being viewed, it's of type `*const T` for non-`mut` slices and `*mut T` for `mut` ones

## User-Defined Types

Users can create `struct` types like so:

```rs
struct Point2D {
    x: f64,
    y: f64
}
```

These act like structures in C.

## Function Pointers

Function pointers are callable, and pointer-sized. They take the same form as functions, but without
the name.

```rs
fn adder(x: i32, y: i32) -> i32 { 
    x + y
}

let f: fn(i32, i32) -> i32 = adder

f(1, 2)
```

## Casting

Objects of a type can be cast to/from other types with `as`.

```rs
let x = 5
let y = x as f64
```

`as` performs "normal" casting. It will truncate, sign extend, 
convert to floating-point and back, and many other things that
you would expect from type casting. However, it doesn't do one
thing: bitcasting.

"Bitcasting" is for unsafe casts, i.e. reinterpreting the bits 
of an object as a type that it isn't. 

`as!` can be used for this when it's desired. It is only *needed*
when casting between pointer types, and it can only be performed
on objects that are the same size beforehand.

```rs
let ptr: *mut byte = nil
let ptr2 = ptr as! *mut Point2D
```