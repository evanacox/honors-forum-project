---
layout: default
title: Linking
nav_order: 8
parent: Compiler Design
---

# Linking

Of course, all the work we've put in to generate object code won't
do very much good unless we were able to actually create `.exe` files from 
that. 

The main LLVM library provides APIs for emitting assembly and object code,
but not for actually *linking* those IR files together.

We've got three main options for linking our object code:

* Invoking `gcc`, `clang`, or something else
* Invoking the system's linker (`ld`, `LINK.exe`, etc)
* Use LLD, the LLVM linker

## The "Cheating" Method

We could just invoke the system's C (or C++) compiler and rely on them
to properly invoke the system's linker for us. `gcc` and `clang` both
will automatically detect object files and will correctly link them
together into an executable. 

This is actually how the Gallium compiler works. It uses the environment variable `$CC` as the compiler's name (note that it actually does rely
on the C++ standard library that the Gallium runtime library was compiled
against to be linked in somehow).

This method is quick, stupid simple and "just works." But you stop being
in complete control of your own link process. If you want anything remotely
advanced, you need to control the linker directly. 

## The "Normal" Method

If you have more time to implement proper linking, you can directly
interact with a system linker. 

In order to do this properly, you need to do three things:

1. Identify the toolchain being targeted 
2. Find the toolchain and any related libraries you need
3. Invoke that linker properly

You need to figure out exactly what you need from the toolchain and
deal with each one individually (as they all have different quirks that
need to be accounted for). Basically, you just need code to deal with
each type of toolchain.

## The LLVM Method

The LLVM project also provides a decently fast linker, `lld`. This can
be used as a drop-in replacement for the linker in the normal method,
but the requirements to deal with each toolchain individually still stand.

`lld` provides several drivers that conform to different interfaces, e.g 
`ld.lld` conforms to the same interface as `ld`, `ld-link` conforms to
the same interface as `LINK.exe`, etc. These can be used as just a 
faster (on average) replacement for the system linker in the above method.

The other method involves directly invoking `lld` through the C++ API.
How to do this is not very well-documented, but it can be found by exploring
through [the LLD source](https://github.com/llvm/llvm-project/tree/main/lld).