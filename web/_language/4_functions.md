---
title: "Functions: Gallium's Workhorse"
permalink: /articles/language/functions/
excerpt: "Gallium's functions, how they work, and what they do differently."
usemathjax: true
classes: wide
---

Functions can be created, called, and passed around. Gallium has first-class function support.

```rs
fn add(x: i32, y: i32) -> i32 {
    x + y
}

fn add_mul(x: i32, y: i32, z: i32) -> i32 {
    add(x, y) * z
} 
```

## Function Pointers

Function pointers exist, and are implicitly created when referring to a function by name (without calling it).

```rs
let f = add
```

## Higher-Order Functions

Functions can be passed around and used to create higher-order functions.

```rs
fn lazily_add(x: i32, f: fn() -> i32) -> i32 {
    x + f()
}

fn get_16() -> i32 {
    16
}

fn add_16(x: i32) -> i32 {
    lazily_add(x, get_16)
}
```
