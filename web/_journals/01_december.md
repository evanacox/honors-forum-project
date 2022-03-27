---
title: "Journal Entry: December"
permalink: /journals/december/
usemathjax: true
classes: wide
---

Over the last month and a bit, I have made massive progress on implementing my compiler. I went from having a semi-working parser/type checker for my language to having a compiler that can successfully produce assembly/machine code based on a given source file written in my language. While it cannot produce actual .exe files yet, the complex part is done. All that it needs now is some “plumbing” code to connect a few separate systems together, plus a few calls into LLVM and the system linker, and I’ll have a usable .exe file. 

This month, I was focused almost entirely on writing code. I wrote code to implement: 
- type-based analysis/overloading for operators and functions 
- handling indirection and memory accesses in the type system
- modularized name resolution and lexical scoping 
- type-checking array indexing, field accesses on record types, other aggregate operations
- encoding and decoding entity names to “mangle” them for later use using a specification that I wrote alongside the encoder/decoder
- LLVM IR generation for functions, global constants, compiler intrinsics and externally-defined functions
- LLVM IR generation for mathematical and logical expressions
- LLVM IR generation for basic control-flow, including loops, if-elif-else, etc.
- LLVM IR generation for memory/aggregate operations, and code that correctly moves data in/out of memory and registers accordingly 
- LLVM IR generation to detect some run-time errors, i.e integer overflow, trying to access out-of-bounds array indices, etc.
- A runtime library that can be linked with the generated code to properly set up the environment for my language, and ensure that it interacts with the OS/libc properly
  
With all of that, I’m currently in the middle of February schedule-wise for all the code. Unfortunately, with my near-exclusive focus on code this month, I didn’t get around to writing much of my technical documentation / non-technical “what even is a compiler?” explanations, so I’m slightly behind schedule on that front. I did write some documentation, but it was more documenting what exactly the compiler did rather than how it did it or why (e.g instead of saying “because of A and B we need to do C, and here’s how that plays into the greater whole” the documentation I wrote this month was more of “we do C, here’s the excruciating detail on exactly what we do.” It ended up being more about documenting the assumptions/specifics of exactly how my compiler made certain more open-ended features work (mostly in order to keep them straight in my own head, this stuff is complicated), rather than about how or why. 

Most of it was surprisingly straightforward (note that I mean most, there were some difficult problems that snuck in where I really didn’t expect them), and I would consider myself easily on-track for completion. The compiler currently implements a usable C-like subset of the language, with all of the core features that I expected to be able to finish by March done. All I really need to focus on in the coming months is refining that writing, documenting the darker corners of the compiler’s codebase, and writing a ton of tests to iron out the hundreds of obscure bugs/oversights I assume are hiding in the new parts of the compiler. 

From this entire process, I’ve learned a huge amount on what actually goes into creating a usable compiler. I’ve learned about the more obscure jobs that a compiler needs to perform, and I’ve learned that there exist much better ways of doing those jobs than the way I did them (unfortunately I only realized this after-the-fact, and I’m not exactly itching to rewrite a few thousand lines of code to make it easier to maintain or to make it perform better). I’ve also learned a lot about all the thought that needs to go into formalizing semantics ahead-of-time, instead of just figuring it out as I got to it: figuring out what needs to happen before-hand and writing code to do that afterwards is much easier than writing a bunch of code, realizing that the original assumptions were wrong, and trying to fix those assumptions while breaking the least amount of other code. 