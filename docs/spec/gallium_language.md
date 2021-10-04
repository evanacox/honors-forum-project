---
layout: default
title: The Gallium Language
nav_order: 1
parent: Specification
---

# Gallium
Gallium is what you get when you start with C++, throw in some Rust-style syntax, and use 40 years of hindsight to make better decisions on defaults. 


~~~ 
import core::traits::Integral

pub fn sum<T: Integral>(array: &[T]) -> T {
  let result = 0 as T 

  for n in array {
    result += n 
  }

  return result 
} 
~~~

## Moves & References
Unlike C++'s move semantics, Gallium uses *destructive* moves based on an affine type system. 

In normal English, this means that an object can be moved once before it is made unavailable for use by the compiler.

~~~
let s = String("Hello")
let s2 = s              // s is moved to s2 here
let n = s.len()         // compile error: use of moved-from object
~~~

References to objects however do not move it:

~~~
let s = String("Hello")
let ref = &s            // type of ref: &String
use_string(ref)         // doesn't move s
let x = s.len()         // still usable 
~~~

### Trivial Types


### References
Unlike Rust, there is no "borrow checker" with references. You are simply expected to not let your references be invalidated. 

## Variables 
Logically, variables in Gallium are *bindings* that bind a value to a name, instead of being *memory locations*. This is a subtle difference, but it explains the semantics. 

Variables are simply names attached to values, and thus are not meant to be mutated by default. 
~~~
let x = 5 
x := 6 // compile error 
~~~

However, not allowing mutation at *all* would be a bit extreme,
so enabling mutability is possible with `var`. 

~~~
var x = 6
x := 3
~~~

In this example, `x` is rebound to 3. 


## Structures & Classes
Unlike C++, `struct` and `class` do not need to be the same thing for compatibility reasons. They actually have a semantic difference, instead of just being the same thing with different default visibility.

### `self`
Like Rust, methods declare how they interact with the current object through their `self` parameter.

There are four forms:

1. `&self`: Takes an immutable reference to `self`, only needs to view the current state

2. `&mut self`: Takes a mutable reference to `self`, needs to (potentially) mutate the current state

3. `self`: Takes `self` by value, therefore moving the current object. It can't *modify* the object, but definitely needs to *move* it. 

4. `mut self`: Like `self`, but allows modification *and* moving. 

~~~ 
fn uses(s: String) -> Foo { ... }

class Thing {
  var thing: usize 

  pub fn check_thing(&self) -> usize {
    // doesn't do anything but view `self`
    return self.thing
  }

  pub fn update_thing(&mut self) {
    // mutates self, needs `mut` 
    self.thing *= 2;
  }

  pub fn foo(self) -> Foo {
    // moves, but doesn't modify
    return uses(self)
  }

  pub fn 
}
~~~

### Visibility
- `pub`: equivalent to `public`. Available to all other code in all other modules 
- `hidd`: Similar to C#'s `internal`, this is available to other code in the same module, but not anything else
- `prot`: equivalent to `protected`. This is available to derived types only 

### Structures

~~~ 
pub struct Point3D {
  int var x: f64
  var y: f64
  var z: f64 
}
~~~

With structures, all data **must** be `pub` (and thus is implicitly `pub`). Methods are allowed, but they are not allowed to take anything but a `&self`.  