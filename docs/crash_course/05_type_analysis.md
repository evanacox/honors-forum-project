---
layout: default
title: "Type Analaysis & Type Systems"
nav_order: 5
parent: Compilers Crash Course
---

# Type Systems: Analyzing and Understanding Programs

As was mentioned in the [High Level Overview]({{ site.baseurl }}{% link ./02_high_level.md %}) section, after we've successfully parsed a program, we need to *analyze* it. We need to ensure that the program actually makes sense in context. 

## Background: What Is a "Type"?

Before we can explain what type-based analysis is and how we do it, we need to establish what a *type* even is. 

You've probably heard before that "computers only understand binary." This is technically correct: the only thing your computer actually operates on is big (as in  lists of 1s and 0s. But, in reality, those sets of 1s and 0s can mean vastly different things depending on the meaning that our *program* assigns to them. 

As an example, let's consider the following 64 0s and 1s (a 0 or a 1 will be called a "bit" from now on):

~~~
0100101001101111011010000110111000100000010001000110111101100101
~~~

I'd wager that this looks pretty meaningless to you, and you're completely right. There is no inherent meaning in these bits! There are quite literally trillions of valid ways we could interpret this data, depending on what *type* of data we pretend it contains. 

If we treat it as containing a single integer, we would get exactly `5,363,620,503,418,597,221`, or approximately 5.3 quintillion.

~~~
0100101001101111011010000110111000100000010001000110111101100101
^                                
5,363,620,503,418,597,221                  
~~~

We could also treat it as containing *two* integers, by making two 32 bit sections and treating each as individual integers. If we do that, we get `1,248,815,214` in the first 32 bits, and `541,355,877` in the second 32 bits.

~~~
01001010011011110110100001101110 00100000010001000110111101100101
^                                ^
1,248,815,214                    541,355,877
~~~

If we treat it as containing text, we could break it into 8 chunks of 8, and treat each chunk as a letter. When we do so, we get the text "`John Doe`":

~~~
01001010 01101111 01101000 01101110 00100000 01000100 01101111 01100101
^        ^        ^        ^        ^        ^        ^        ^
J        O        H        N        <space>  D        O        E
~~~

> No, I am not making this up. That is actually the exact bit pattern that contains "`John Doe`" on your computer, at least in some programs.

Point is, there are many different ways of interpreting these bits. The list above isn't exhaustive of course: when you make up the rules, you can say that those bits mean literally anything you want! Now, consider that your computer could be operating on quite literally *billions* of these at a time, and that the example shown is only 64 of them. It's just not reasonable for a human programmer to "just remember" what everything is! 

And that is where types come in. Types give us a way to categorize these bits, and say that "this section of bits represent text," or "this section of bits represents a 3D coordinate" or "these bits represent a fraction" or "these bits represent a single musical note." 

When we pair types with a programming language, they give us a way to *automate* all of this bit-hackery. Instead of thinking about programs as "these bits contain \<something\>", we can think of it as "we have a value of \<some type\>" that contains \<some value\>.  

## Type Analysis

Now that we've established what a type is, how do we actually integrate this into a language? 

Recall back to the last section, where we discussed an "AST." We'll be using a very similar type of AST for this article, with one slight modification: we will have values that model text, they will be represented as a value `"With something inside quotes, like this"`.

Consider the following AST: 