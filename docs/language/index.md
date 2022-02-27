---
layout: default
title: Gallium Language
nav_order: 4
has_children: true
---

# Gallium Language

This is the documentation about the language itself, at least as far as 
the compiler implements it.

## Quick Examples

These are taken out of the `compiler/tests/` directory of the repository, so
they work.

### Hello, World!

As always, a "Hello, World!" program is necessary.

```rs
fn main() -> i32 {
    println("Hello, World!")
}
```

### Fizzbuzz

Prints out the first 100 "fizzbuzz" numbers. Between 0 and 100, print out every number, but
if the number is divisible by 3, print "Fizz," print "Buzz" if it's divisible by 5, and print both
if it's divisible by 3 and 5.

```rs
fn main() -> i32 {
    for i := 0 to 100 {
        print(i)

        if (i % 3 == 0) and (i % 5 == 0) {
            println(": FizzBuzz!")
        } elif i % 3 == 0 {
            println(": Fizz!")
        } elif i % 5 == 0 {
            println(": Buzz!")
        } else {
            println("")
        }
    }

    0
}
```

### Distance Formula

This calculates the distance between two points, using the system's C library for
`sqrt` and `pow`.

```rs
external {
    fn sqrt(x: f64) -> f64
    fn pow(x: f64, exp: f64) -> f64 
}

struct Point2D {
    x: f64
    y: f64
}

fn distance(a: Point2D, b: Point2D) -> f64 {
    let x2 = pow(b.x - a.x, 2.0)
    let y2 = pow(b.y - a.y, 2.0)

    sqrt(x2 + y2) 
}
```

### Palindrome

This checks a "string" (character array) to see if it represents a palindrome.

```rs
fn palindrome(s: [char]) -> bool {
    for i := 0 to s.size / 2 {
        if s[i] != s[s.size - i - 1] {
            return false 
        }
    }

    true
}
```

### Slices & Slice Notation

This gives an example of arrays when used with slice notation.

```rs
fn array_eq(a: [i64], b: [i64]) -> bool {
    if a.size != b.size {
        return false
    }

    for i := 0 to a.size {
        if a[i] != b[i] {
            return false 
        }
    }

    true
}

fn main() -> i32 {
    let a1 = [1, 2, 3, 4, 5]
    let a2 = [3, 4, 5]
    
    assert array_eq(&a2, a1[2..5])

    0
}
```

### Dynamic Allocation

This example uses the system's C library to dynamically allocate and deallocate memory,
in this case an array of structures.

```rs
external {
    fn malloc(size: usize) -> *mut byte
    fn free(ptr: *mut byte) -> void
}

struct Point3D {
    x: f64
    y: f64
    z: f64
}

fn allocate(size: isize) -> [mut Point3D] {
    let size_bytes = (size * sizeof Point3D) as usize
    let ptr = ::malloc(size_bytes) as! *mut Point3D

    [ptr len size]
}

fn deallocate(slice: [mut Point3D]) -> void {
    ::free(slice.data as! *mut byte)
}

fn main() -> i32 {
    let data = allocate(5)

    for i := 0 to data.size {
        data[i] := Point3D { 
            x: (i as f64) + 1.0, 
            y: (i as f64) + 2.0, 
            z: (i as f64) + 3.0 
        }
    }

    deallocate(data)

    0
}
```