---
layout: default
title: Programming Languages Aren't Magic
nav_order: 3
parent: Language Design
---

# Programming Languages Aren't Magic
When you write code, how do you write it? Probably by typing 
source code into an editor and hitting a "run" button, or by
running some commands in a terminal that make your code go. 

Maybe you've written some Java code like the following:

{% highlight java %}
public static int multiply(int x, int y) {
  return x * y;
}
{% endhighlight %}

Or, maybe you learned Python and wrote something like this:

{% highlight python %}
def multiply(x, y):
  return x * y
{% endhighlight %}

Whatever the case is, you were writing a glorified text file,
and *somehow*, the computer understood what it was and what to do with it.

But how?

## Computers Understand Instructions
Unfortunately, it's not as simple as "the computer understands what `x * y` means," 
because it actually doesn't! What your computer *actually* understands looks more
akin to this:

{% highlight nasm %}
137 248 15 175 199 195
{% endhighlight %}

Those numbers correspond to specific *machine instructions*, which are
the operations that computers can actually perform. These 
are *incredibly* low level, and must contain every precise detail of what needs to happen at the machine level. 

Examples:
  - "copy a value from one location to another"
  - "multiply the value at these 2 locations" 
  - "compare the values at these 2 locations"
  - "start executing the instructions at another location"
 
Humans used to actually have to write programs with these
machine instructions, and people realized fairly quickly
that this was too tedious and too error-prone to be sustainable
in the long term; thus, we began to try to find ways to avoid
writing these. 

## Two Approaches to the Same Problem
Two competing solutions to this fundamental problem arose.

1. Translate something that *humans* understand into something that the *computer* understands
    - Think of it like a translator sitting down and transcribing a document into another language 

2. Make a program that can both understands something *humans* understand and knows how to *interpret* it for the computer.
    - Think of it like an interpreter listening to what you say and then repeating it in another language for someone else 

We call the former a *compiler* and the latter an *interpreter*. 
The goal of these programs is simple: take *something* that the humans 
understand and can produce to tell the computer what to do, and 
use that to actually make a usable program. 

This is much easier said than done. 

## Source Code -> Machine Code 
Modern tools have mostly settled on one approach, taking a text file
that contains text that happens to be formatted a certain way, 
and using that to produce a program. 
