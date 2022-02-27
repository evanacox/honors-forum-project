---
layout: default
title: Expressions & Statements
nav_order: 2
parent: Gallium Language
---

# Expressions & Statements

Gallium is expression-based, meaning that almost every construct in the language is 
an expression and can be used as such. 

## Blocks

Blocks are created with `{}` and implicitly evaluate to the last expression that was
evaluated inside of them.

```rs
let a = {
    print("Hello!")

    1 + 1
}

// a == 2
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

## Variables

Variables can be created with `mut` or `let`.

`let` is immutable, `mut` is mutable.

```rs
mut x = 5
x := 6 // fine

let y = 5
y := 6 // compiler error
```

## Assertions

Assertions can be done with the `assert` statement, which
holds a boolean condition and an optional message.

If the condition is false, the program aborts instantly.

```python
assert 1 == 1, "did math break?"
assert true and not false
```

## Literals

### `nil`

`nil` can be used as a literal for a pointer of any type.

```rs
let x: *mut byte = nil
```

### Numbers

Any number can be a literal for a numeric type, by default
they will coerce to any type they fit in. If not coerced,
they become `i64`.

```rs
let x = 5 // x: i64
```

Floating-point literals need a `.` somewhere. They are `f64`.

```rs
let x = 3.141592
```

### Characters & Strings

Character literals must fit inside a single byte.

```rs
let c = '!'
```

String literals become a magic compiler-generated `[char]` that
point to a magic compiler-generated string that is always
accessible for the lifetime of the program.

```rs
let s = "Hello!"
```

### Arrays

Arrays can be created with `[]` as expected.

```rs
let arr = [1, 2, 3, 4, 5]
```

### Slices

Slices can be created through range notation (explained in #1)
or from raw storage + a length.

```rs
let raw: *mut i64 = malloc(512 * sizeof i64) as! *mut i64
let slice: [mut i64] = [raw len 512]
```

### Structures

Structures can be initialized with literals.

```rs
struct Point2D {
    x: f64
    y: f64
}

let origin = Point2D { x: 0.0, y: 0.0 }
```

