---
title: "Parsing Methods"
permalink: /articles/design/parsing/
excerpt: "How compilers can deal with the parsing process."
usemathjax: true
toc: true
---

For any programming language, the first step is making sense of the actual
text that programmers need to write. 

For starters, I would suggest looking these two pages:

- [Parsing (from 'Crafting Interpreters')](https://craftinginterpreters.com/parsing-expressions.html)
- [Grammars (from 'Crafting Interpreters')](https://craftinginterpreters.com/representing-code.html)

Once you understand the gist of those two articles, you can consider the two
paths for your compiler's parser.

## Generated Lexer/Parsers

"Generated" parsers are exactly what they sound like. Instead of hand-writing code
that lexes and parses text, you provide a specially-formatted file that models
the grammar of the language, that file is then compiled **into** a lexer/parser. 

The Gallium compiler uses ANTLR internally, but there are a wide variety of
other tools that fulfill the same roles. Just Google "parser generator {language}"
and you'll find plenty.

As an example, consider the following ANTLR grammar:

```
DIGIT
    : [0-9]+
    ;

expr
    : expr ('*' | '/' | '%') expr
    | expr ('+' | '-') expr
    | DIGIT
    ;
```

These are extremely quick to produce, modify and get into a working state. However,
they are most definitely not known for good error messages. You will get a 
"mismatched token!" error at the first instance of a syntax error, and not report
anything after that for the most part. 

They are great for prototyping, but for anything that's meant for wide appeal
they are probably not the best idea.

## Hand-written Lexer/Parsers

The other option is pretty self-explanatory, and is the approach given in
Crafting Interpreters. 

The usual approach for decent performance and decent error reporting is
'recursive descent,' and is a series of recursive function calls that
parse small pieces of the grammar at once. 

Since these are hand-generated, good error messages for common erroneous inputs
can be added in, and in general errors can be made to be much better than their
generated counterparts can offer. 

However, they are much more difficult to actually *modify* the language during
language development.

## Conclusions

My advice would be to use a generated parser while the syntax of the
language is still in flux, and create a more permanent hand-written parser once
the syntax has solidified.