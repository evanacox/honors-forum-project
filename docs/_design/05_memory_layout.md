---
title: "Memory Layout"
permalink: /articles/design/layout/
excerpt: "The memory layout of Gallium types, and its interaction with LLVM."
usemathjax: true
toc: true
---

## Arrays
Arrays are treated similarly to arrays in C/++, they don't keep track of their own size *at run-time*. Array sizes are encoded at the type level, at run-time they are simply a contiguous list of objects. 

In LLVM, these are mapped directly to arrays. For example, an array `[T; 64]` would map to `[T x 64]` in LLVM IR. 

## Structures
The memory layout of structures are treated similarly to how they are in C, except for one key difference: the ordering of fields is not guaranteed to be preserved. Currently, fields are re-ordered in order to minimize padding requirements, but nothing can be relied upon. 

They are mapped directly to `type`s at the LLVM IR level, and LLVM handles padding and whatnot. For example, consider the following `struct`:

~~~ 
struct DynamicBuffer {
  ptr: *mut byte
  size: usize
  cap: usize
}
~~~

One possible memory layout (on a 64-bit architecture) could be:

~~~ llvm
%DynamicBuffer = type { i8*, i64, i64 }
~~~

> Note: The current implementation minimizes padding by sorting fields largest-to-smallest (according to their size) to put smaller fields in the area that would be padding otherwise, but the relative order of **same-sized fields** is preserved with a stable-sort algorithm. 
> **This is not guaranteed to continue**.

## Slices
Given a slice `[T]` (or `[mut T]`, the layout is the same), a structure equivalent to the following Gallium struct is generated:

~~~ 
struct __SliceT {
  __ptr: *mut T
  __size: isize
}
~~~

This would be translated to this in LLVM (for a 64-bit architecture):

~~~ llvm
%Slice = type { T*, i64 }
~~~

In this particular case, the order is pointer first and size later, in order to not pessimize the common case of simply indexing into the array. 

The actual layout at runtime would look something like this:

![layout #1]({{ site.url }}{{ site.baseurl }}/assets/images/compiler-design/layout/slice-layout-1.png)

Note that slices don't have to point to the beginning of an array, they could also look like this at runtime:

![layout #2]({{ site.url }}{{ site.baseurl }}/assets/images/compiler-design/layout/slice-layout-2.png)