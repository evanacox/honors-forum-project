---
layout: default
title: Compiler Builtins
nav_order: 6
parent: Compiler Design
---

# Builtins
Gallium defines a set of runtime functions and builtins that come from various
places in the code, these symbols are all under the reserved `__` prefix.

## `extern "C" [[noreturn]] void __gallium_trap() noexcept`
Instant termination function, used for a quick and abnormal exit that usually
will get a debugger to that location. Skips the C runtime or any other
code and instantly aborts, usually with an instruction rather than a call.

### Notes
- Callable in user code with `__builtin_trap`

- Implemented by emitting a `weak` function in each LLVM module, relies on `@llvm.trap`