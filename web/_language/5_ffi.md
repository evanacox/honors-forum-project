---
title: "FFI: Communicating with the Outside World"
permalink: /articles/language/ffi/
excerpt: "How Gallium handles C FFI."
usemathjax: true
classes: wide
---

FFI is how any language actually interactions with the outside world after all, 
so Gallium has it.

## Calling Gallium over C FFI

Gallium functions are normally mangled and thus not visible over FFI (easily).

When marked `extern` however, they are not mangled, and can be called over FFI.

```rs
extern fn add(x: usize, y: usize) -> usize {
    x + y
}
```

The above function could be called from C++ like so:

```cpp
extern "C" {
    std::size_t add(std::size_t x, std::size_t y);
}

int main() {
    auto x = add(5, 3);
}
```

## Calling C from Gallium over C FFI

Gallium can also call functions over the C FFI through the use of `external` blocks.

```rs
external {
    fn malloc(size: usize) -> *mut byte
    fn free(ptr: *mut byte) -> void
}

fn use_malloc() {
    let ptr = malloc(512)

    free(ptr)
}
```

C FFI is actually how `print` and friends work, they talk over C FFI to some C++ code in
the Gallium runtime library.