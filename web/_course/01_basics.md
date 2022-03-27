---
title: "Programming Languages Aren't Magic"
permalink: /articles/crashcourse/basics/
excerpt: "What a compiler is, why we need them, why they matter."
usemathjax: true
classes: wide
---

When humans program, we are almost always are writing in what's called
a *programming language*. This is exactly what it sounds like: a 
language used for programming. But why? Why can't we just program in English
and tell the computer exactly what we want?

## Programming Languages
If you've programmed, consider the following Java code:

~~~ cs
public static int f(int x) {
  return (x * 2) + 6;
}
~~~

Or equivalent Python code:

~~~ python
def f(x):
  return (x * 2) + 6
~~~

If you haven't, maybe you just wrote this into a calculator and somehow the calculator understood it (calculators are computers too!):

$$ 2x + 6 $$

Whatever the case is, you were writing a glorified text file,
and *somehow*, the computer understood what it was and what to do with it.

But how?

## Computers Understand Instructions
Unfortunately, it's not as simple as "the computer understands what `(x * 2) + 6` means," because it actually doesn't! What your computer *actually* understands looks more akin to this:

~~~ nasm
72 141 68 63 6 195
~~~

> Yes, this is actually the machine code for a function returning `(x * 2) + 6` on Intel/AMD CPUs, I promise I didn't just make up those numbers.

Those numbers correspond to specific *machine instructions*, which are the operations that computers can actually perform. These are very low level, and must contain every precise detail of what needs to happen at the *machine* level. 

Examples:
  - "copy a value from one location to another"
  - "multiply these two values" 
  - "check if this value is greater than that value"
  - "add these two values"
  - "update this location to contain this new value"
  - "swap the values at these 2 locations" 

To do anything remotely complicated, many of these instructions must be put together in a list that the computer can execute one-by-one. 
 
Depending on the computer, these instructions can be arbitrarily complicated or arbitrarily simple. For example, your desktop or laptop probably has an instruction to calculate sine/cosine for a radian value, while the computer chip inside a digital picture frame probably doesn't even have a multiply instruction!

Humans used to actually have to write programs with these
machine instructions, and people realized fairly quickly
that this was too tedious and too error-prone to be sustainable
in the long term; thus, we began to try to find ways to avoid
writing them. 

What we needed was a way to describe what we wanted the computer to do, but in a way that both humans *and* computers could understand. 

## Two Approaches to the Same Problem
Two competing solutions to this problem arose.

1. Translate something that *humans* understand into something that the *computer* understands ahead-of-time
    - Think of it like a translator sitting down and transcribing a document into another language 

2. Make a program that can both understands something *humans* understand and knows how to *interpret* it for the computer, usually on-demand. 
    - Think of it like an interpreter listening to what you say and then repeating it in another language for someone else 

We call the former a *compiler* and the latter an *interpreter*. 

## Why Source Code?
Modern tools have mostly settled on one approach, taking a text file that contains text that happens to be formatted a certain way, and using that to produce a program. 

Text formats are not the only way of doing this, nor is there anything particularly special about text. Text just happens to be ubiquitous across every operating system, making it much simpler to share code between the variety of systems that exist. 

The same translation process described above happens in "visual" environments like Scratch or Unreal Engine's Blueprints, just in slightly different ways. 

![Forms of Code](/assets/images/crash-course/whats-a-compiler/forms-of-code.png)

They're all just methods of making the computer do what we want it to do, after all! 