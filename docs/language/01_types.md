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

## Compound Types

- `[T]` / `[mut T]`: Slices, also known as "array views." They are a reference to part of an array
- `[T; N]`: Statically-sized arrays

## User-Defined Types

Users can create `struct` types like so:

```rs
struct Point2D {
    x: f64,
    y: f64
}
```