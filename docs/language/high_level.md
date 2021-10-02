---
layout: default
title: High Level Overview of a Compiler's Job
nav_order: 2
parent: Language Design
---

# High Level Overview of a Compiler's Job
In the previous article, we explored the fundamental problem of programming
computers: the computers don't understand the humans, and vice versa. 

We also explored the idea of a compiler vs. an interpreter. 

## Overall Job
If you recall, the basic goal of a compiler is to translate 
what a human can understand to something that a computer can
understand. 

The following is an extremely high-level view of the phases a compiler has to go through (note that each of these terms/phases will be explained later):

![Overview of Compiling](../assets/images/compiler-phases-transparent.png)

While that may look like a lot (or it may not, you never know), a compiler at its core is simply
trying to turn text that humans can understand into different forms that the computer can understand, all of which help the compiler to do different jobs. 

Just like we have different ways of representing human ideas (words, math notation, images, etc.), the compiler has different ways of representing ideas that help it to better do its job. 

![Human v. Machine Readable](../assets/images/human-v-machine-readable-transparent.png)

## Phases
Articles going into much more depth on each of these will be published later, these are the 10,000ft view of the idea. 

### Source Code
As mentioned in the previous article, source code is quite literally text in a `.txt` file, just with a different file name. 

In order for a compiler to understand that code however, it must follow some sort of defined grammatical and semantic rules that the compiler is able to understand.

Think of it like English, where "walked home Robert" is nonsensical while "Robert walked home" is valid and understandable. 

### Parsing
"Parsing" is effectively computer jargon for "reading and making sense of text," in this case the computer is reading in the source code that a human wrote, and trying to figure out what the source code means. Once it figures that out, it turns it into a representation that the compiler can then begin to actually operate on. 

Think of this like breaking down an English sentence into subject(s), verbs operating on those subjects, and other parts of a sentence, and figuring out what each part refers to/means.

This representation is typically called an "abstract parse tree," also known as an AST (AST is easier to type). 

### Analysis
This step varies wildly depending on the programming language / compiler, but in general this amounts to "making sure the code makes sense." 

Think of it like in English, many sentences are *grammatically* sound, but are nonsensical ("Colorless green ideas sleep furiously," for example). The analysis step filters out those grammatically correct but nonsensical programs. 

Typically, this step does not modify the AST. 

### Intermediary Representation (IR) 
Like the name implies, this is a representation that is an intermediary. In this case, it acts as an intermediary step between machine code and the representation that the parser created. 

Machine code is obscenely pedantic and low-level, and the parser's representation is meant to be abstract and (relatively) simple. Translating between these two is possible, but it is fairly error-prone and ends up being difficult to modify later due to complexity. In order to simplify the translation, the IR tries to be somewhere in the middle: more low-level and pedantic than the parser's representation, but not as low-level as machine code. 

The parser's representation can then be translated into the IR in a (more) straightforward way, and then the IR can be translated to machine code in a (more) straightforward way. 

### Optimization
Many IRs have been created over the years that lend themselves to automated optimization, enabling a "smart" compiler to rewrite parts of a program in order to make them simpler and faster for the computer to actually execute. 

These tricks can be math trickery, removing unnecessary calculations, or much more complex and wider-reaching changes that I can't explain in a sentence. 

This step is where the majority of current compiler research is going, as better optimizers means faster programs, and faster programs means saved time, less money spent on electricity, and happier users. 

> Note: The electricity & time benefits alone could amount to billions of dollars saved in the long run at the scale of companies like Google; there's a reason they pay people $150k/year to make compilers better at optimization)

### Machine Code
Finally, this step is the result of the compiler: a program the computer understands. 

The IR is fully translated into machine code, and then that machine code is put into file (usually an `.exe`) that a user can then execute. 