---
layout: default
title: Grammar & Syntax
nav_order: 2
parent: Specification
---

# Grammar

Basically, just go look at [this file](https://github.com/evanacox/honors-forum-project/blob/master/compiler/src/syntax/Gallium.g4)
for the formal grammar.

# General Syntax Ideals

- no `;`s, whitespace and newlines are used to disambiguate where necessary

- try to avoid context-sensitivity as much as possible

- Left-to-right, with the "general" form of `entity name: type`

    - `let thing = foo` 