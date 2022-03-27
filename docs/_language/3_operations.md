---
title: "Operators & Operations"
permalink: /articles/language/operators/
excerpt: "Gallium's set of operators, and the operations that they do."
usemathjax: true
classes: wide
---

# Operators and Operations

Instances of a given type have operations they're able to do, as with most any language.

## Arithmetic

The typical `+`, `-`, `*`, `/` and `%` exist. They have the expected semantics,
they operate on integers for integral types (i.e. rounded toward zero for division),
and for floating-point they follow the IEEE standard (i.e. they work like every other
language).

```rs
let x = 5 + 3
let y = x * (12 + x)
let z = sqrt(3.0)
```

## Bitwise

The bitwise operations also work as expected. `&`, `|`, `^` have their standard behavior,
and work as they should with two's complement numbers. 

The left shift operator (`<<`) works as expected, new bits are zero and bits shifted out disappear.
The result of `x << y` is precisely $$ x * 2^y mod 2^N $$ where $$ N $$ is the number of bits
in the integer type.

The right shift operator (`>>`) is an arithmetic right shift (shift with sign bit retained) on signed 
integers and a logical shift right on unsigned (shift with zero fill). It also works as is expected.

Negation is done with `~`, again it works as-expected.

```rs
let a = 128
let b = (~a) >> 13
let c = b ^ 1395
```

## Logical

Logical operations use keywords instead of symbols. 

- `a and b`: True if `a` and `b` are true.
- `a or b`: True if `a` or `b` is true.
- `a xor b`: True if exactly one of `a` and `b` is true. 
- `not a`: True if `a` is false

```rs
if a and not b and (c or d) {
    print("ok")
}
```

## Array

Arrays are accessed with the subscript operator, `[]`.

```rs
let a = [1, 2, 3]

print(a[2])
```

Arrays (and slices) can be "sliced" with `[]` as well. This creates a slice that refers
to a subsequence of the array from a particular index to a particular end index. There are 
two slice patterns:

- `a..b`: Exclusive slice. Takes every index starting at `a` and up to **but not including** `b`
- `a..=b`: Inclusive slice. Takes every index starting at `a` and up to and including `b`

```rs
let a = [1, 2, 3, 4, 5]

// sub is viewing a[0] through a[2]
// sub is equivalent to `[1, 2, 3]`
let sub = a[0..3] 

// sub2 is viewing a[0] through a[3]
// sub2 is equivalent to `[1, 2, 3, 4]`
let sub2 = a[0..=3] 

// sub3 is viewing a[3] through a[a.size - 1]
// sub3 is equivalent to `[4, 5]`
let sub3 = a[3..a.size] 
```

For more information on this idea, see Python's slices or Rust's range notation.

## Structures

Structures can have their fields accessed with `.` and can be created with struct initializers.

```rs
struct F {
    a: i32
}

let object = F { a: 12 as i32 }
let b = object.a 
```