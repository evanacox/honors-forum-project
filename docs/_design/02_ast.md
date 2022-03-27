---
title: "Gallium AST"
permalink: /articles/design/ast/
excerpt: "The AST of the Gallium compiler."
usemathjax: true
classes: wide
---

Gallium's AST is a fairly simple class hierarchy. 

![AST Hierarchy]({{ site.url}}{{ site.baseurl }}//images/compiler-design/ast/ast-hierarchy.png)

The base `Node` interface provides access to source location info for each
node, and not much else. 

The actual useful information is provided by the direct bases of `Node`.

Expressions, statements, and declarations have an `enum class` discriminator
that can be used for "RTTI." They also provide equality, 
visiting, and cloning. 

## Visitors

The visitor interface is relatively simple. A visitor type has to both 
implement the interface of one of the user-facing visitors, and possibly
inherit from `ValueVisitor<T>` in some way to provide "returning" non-`void`
values from those visitors. 

Each "category" of type (exprs, stmts, decls, types) provides two main
types of visitors:

- `Const{Category}VisitorBase`: Visited a `const T&` of each node
- `{Category}VisitorBase`: Visited a `T*` of each node

This allows `const`-correctness where possible. 

Each category also includes an alias, `(Const)?{Category}Visitor<T>` that
inherits from `ValueVisitor<T>` and provides the interface for returning 
values.

The AST types themselves implement a protected virtual `internal_accept` 
method that takes a mutable pointer to a "base" visitor. 

### `Any` visitors

![Any Visitor]({{ site.url}}{{ site.baseurl }}//images/compiler-design/ast/visitor-hierarchy.png)

There is also a pair of visitors that can visit *any* node, namely 
`ConstAnyVisitor` and `AnyVisitor` (note the lack of `Base`). These inherit 
from the correct visitor base class for each category, and simply expose 
their APIs through public inheritance.

However, there is additional functionality in the `Base` variants of the
above two types: they provide "just visit all child nodes and do nothing."
They provide an `override` for every visit method, and simply visit
all of the children of the nodes they visit.

In order to not throw away this functionality when a method is overridden,
they also provide a protected `accept` method that will visit whatever
node was passed in with the "just visit the children" behavior.

> *Note: This behavior couldn't really be provided without the ability*
> *of any particular instance to be able to visit **every** child node*
> *without having to just copy the implementation multiple times*