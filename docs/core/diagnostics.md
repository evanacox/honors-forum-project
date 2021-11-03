---
layout: default
title: Diagnostic Reporting
nav_order: 3
parent: Compiler Design
---

~~~
error [E#403] :: mismatched return type in return statement 
  >>> file.gal:6:10
    |
  1 | fn foo() -> i32 {
    |             --- expected type `i32` based on function signature
   ...
    |
  6 |   return 0.4
    |          ~~~ expression is of type `f64` instead of `i32` 
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
