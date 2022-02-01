---
layout: default
title: Compiler Builtins 
nav_order: 6 
parent: Compiler Design
---

# Builtins

Gallium defines a set of runtime functions and builtins that come from various places in the code, these symbols are all
under the reserved `__` prefix.

> *Note: while these may be expressed in C++ syntax, they are not all implemented as*
> *actual functions in the C++ `galrt` library*.

## `extern "C" [[noreturn]] void __gallium_trap() noexcept`

Instant termination function, used for a quick and abnormal exit that usually will get a debugger to that location.
Skips the C runtime or any other code and instantly aborts, usually with an instruction rather than a call.

### Notes

- Callable in user code with `__builtin_trap`
- Implemented by emitting a `weak_odr` function in each LLVM module, relies on `@llvm.trap`.

## `extern "C" void __gallium_string_ptr(s: [u8]) -> *const u8`

Extracts the pointer portion of a string slice and returns it

### Notes

- Callable in user code as `__builtin_string_len`
- Implemented entirely in LLVM IR

## `extern "C" void __gallium_string_len(s: [u8]) -> usize`

Extracts the length portion of a string slice and returns it

### Notes

- Callable in user code as `__builtin_string_len`
- Implemented entirely in LLVM IR

## `extern "C" void __gallium_black_box(std::uint8_t*) noexcept`

Informs the optimizer that anything could happen to the pointer passed in, and that it should assume it has been
modified in any well-defined way.

## Notes

- Callable in user code with `__builtin_black_box`
- Implemented by emitting a `weak` function in each module, uses the inline-asm trick that forces the optimizer to
  assume that the value has been modified in some unknown way