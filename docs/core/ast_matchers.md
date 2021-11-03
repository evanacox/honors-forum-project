---
layout: default
title: AST Matchers
nav_order: 2
parent: Compiler Design
---

All matchers get registered before an AST walking pass that checks
each node for the **beginning** of a match.

For example, say the following was being matched:

~~~ cpp 
auto pattern = match::fn_decl(
  match::has_any_param(
    match::with_type(ast::Type(...))
  )
);

auto matcher = match::Matcher();
matcher.register(pattern);
matcher.walk(tree);
~~~

When the matching pass hits an `fn` declaration, it would
see that a matcher was registered for that, and would begin 
passing itself into the pattern to match the rest. Imagine
the visitor for the tree walking just checking a table to see if
a matcher is registered for a type of node, and if it is, fetching
the patterns and matching. 

Pattern matchers could work something like:

~~~ cpp 
// inside of a visitor type
void visit(const FnDecl& node) {
  if (has_pattern(AstNodeType::fn_decl)) {
    for (auto& pattern : matchers(AstNodeType::fn_decl)) {
      // check if it matches, do whatever if it does
    }
  }
}

// maybe a .matches on a pattern object 
bool matches(const SomeNode& node) {
  return node.thing() == 3 && matches_other(node.part());
}
~~~
