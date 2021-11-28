---
layout: default
title: Expressions
nav_order: 3
parent: Specification
---

# Expressions
Expressions in Gallium are very similar to other C-like languages, with a few changes:

## Assignment
Rebinding is now `:=` for non-compound rebinding, to help differentiate it from `==`. 

`=` is no longer valid in an expression, either it needs to be 
the rebinding operator or `==`. `=` is only valid by itself for variable binding. 

## Logical Operators
Logical operators are keywords to help differentiate them from bitwise operators:

~~~
if a and not b {
  // ...
} elif b or d {
  // ...
} elif a xor b {
  // ...
}
~~~

`not a` replaces `!a`, `a and b` replaces `a && b`, `a or b` replaces `a || b`, and a new operator `xor` has been added.

`xor` is a logical XOR, and `a xor b` is exactly equivalent to `not a != not b` for `bool`s. 

## "Everything is an Expression"
By default, nearly every construct in the language is an expression. 

This has several interesting side effects

### Blocks
Blocks evaluate to the last expression in them

```
let result = {
  let x = ... 
  let y = ...
  
  (y * 5 + 3.3352835)
}
```

### If-Else
If-else chains evaluate to whatever the block that was executed evaluates to.

Mind you, if there isn't an `else`, they are not considered "evaluable."

```
let result = if thing {
  5
} elif thing2 {
  6
} else {
  7
}
```

### If-Then
There is an extra form of `if` provided to avoid the line noise of `if thing { a } else { b }`

```
let result = if thing 
  then a 
  else b
```


## Dereferencing
### Auto-Dereferencing
If you have a reference to a type (i.e a `&T` or a `&mut T`) and
you use `.` on it, dereferencing is performed automatically.

~~~
let ref: &Point = ...;

// exactly the same as (*ref).x and (*ref).y
use(ref.x, ref.y)
~~~

Note that this is **not** performed for pointers. For pointers, dereferencing manually is required. 

## Precedence
Bitwise precedence has been fixed, so `a & b == mask` is equivalent to `(a & b) == mask` instead of `a & (b == mask)`.

Precedence is highest-to-lowest

| Expression | Description | Associates |
|---|---|---|
| *Type*()<br>a`()`<br>a`[]`<br>a`.`thing | Constructor Call<br>Function Call<br>Subscript<br>Member Access | Left |
| `not` a<br/>`~`a<br/>`&`a<br/>`&mut` a<br>`*`a | Logical NOT<br/>Bitwise NOT<br>Reference-to or Address-of<br/>Mut reference-to/addr-of<br/>Indirection | Right |
| a `*` b<br>a `/` b<br>a `%` b | Multiplication<br>Division<br>Remainder | Left |
| a `+` b<br>a `-` b | Addition<br>Subtraction | Left |
| a `<<` b<br>a `>>` b | Bitwise left-shift<br>Bitwise right-shift | Left |
| a `&` b | Bitwise AND | Left |
| a `^` b | Bitwise XOR | Left |
| a `\|` b | Bitwise OR | Left |
| a `and` b | Logical AND| Left |
| a `or` b | Logical OR | Left |
| a `xor` b | Logical XOR | Left |
| a `<` b<br>a `>` b<br>a `<=` b<br>a `>=` b | Less-than<br>Greater-than<br>Less-than-or-equal-to<br>Greater-than-or-equal-to | Left |
| a `==` b<br>a `!=` b | Equality<br>Inequality | Left |
| a `:=` b<br>a `+=` b<br>a `-=` b<br>a `*=` b<br>a `/=` b<br>a `%=` b<br>a `<<=` b<br>a `>>=` b<br>a `&=` b<br>a `^=` b<br>a `\|=` b | Rebinding<br>Compound rebinding | Right |
| `if` a `then` b `else` c |  | n/a |
| `match` a |  | n/a |
