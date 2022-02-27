---
layout: default
title: Expressions
nav_order: 2
parent: Gallium Language
---

# Expressions

Gallium is expression-based, meaning that almost every construct in the language is 
an expression and can be used as such. 

## Blocks

Blocks are created with `{}` and implicitly evaluate to the last expression that was
evaluated inside of them.

```rs
let a = {
    if thing {
        foo()
    } elif other_thing {
        bar()
    } else {
        baz()
    }
}
```

## Control Flow

### `if`

If takes two forms, `if-then` and `if-else`. 

```rs
let a = if condition then a else b  
```

The result of an `if-else` expression can only be used if there is an `else` clause.

```rs
let a = if thing {
    b
} else {
    c
}
```

### Loops

#### `loop`

An infinite loop. These are able to evaluate to something when `break` is evaluated.

```rs
let a = loop {
    if condition() {
        break 5
    }
}

// a = 5
```

#### `while`

A loop that goes until a condition is not `true`.

```rs
while !thing {
    use(&mut thing)
}
```

#### `for`

A for loop that goes from `x` to `y`. It can go backwards or forwards.

```rs
for i := 0 to 10 {
    // evaluated for [0, 9]
}
```

```rs
for i := 10 downto 0 {
    // evaluated for [10, 1]
}
```