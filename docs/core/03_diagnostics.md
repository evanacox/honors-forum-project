---
layout: default
title: Diagnostic Reporting
nav_order: 3
parent: Compiler Design
---

# Diagnostic Reporting

Good error messages is one of the most important things a compiler can have in them. After
all, programmers make a **lot** of mistakes. Being able to easily see exactly what went wrong,
where it went wrong, and (ideally) how to fix it removes a lot of the friction of writing code.

~~~
error [E#403] mismatched return type
  >>> file.gal:6:10
    |
  1 | fn foo() -> i32 {
    |             --- expected type `i32` based on function signature
   ...
    |
  6 |   return 0.4
    |          ~~~ expression is of type `f64` instead of `i32` 
    |
  note: return
~~~

~~~
error [E#0494] :: method signature incompatible with trait method 
  >>> file.gal:2668:4
       |
  2668 |     fn print(&mut self, printer: &dyn Writeable) -> void { 
       |              ^^^^^^^^^ trait signature uses `&self`, not `&mut self`
       |
  note: full trait signature is `fn print(&self, &dyn Writeable) -> void` 
~~~
