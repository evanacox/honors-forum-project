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
~~~

## API

Diagnostics are reported through a `DiagnosticReporter*`, an abstract 
interface that takes in diagnostic objects and reports them *somehow* (the
method is irrelevant to the usage). 

> *Note: as of right now there is only a console reporter, but in theory*
> *the errors could be reported through LSP or an IDE's API.*

Diagnostics themselves are considered a set of `DiagnosticPart`s. Each part
represents a single portion of an error message, e.g a single note message,
a list of source locations to point out somehow, etc. These are all
flagged with the *type* of diagnostic (e.g warning, note, error) and
any additional information specific to that type of `DiagnosticPart`.

These are created by various parts of the compiler and then fed into a
reporter when they are encountered. It is up to the reporter to enforce any
ordering invariants. 

### Files

The error reporting infrastructure is located in `compiler/src/errors`.

