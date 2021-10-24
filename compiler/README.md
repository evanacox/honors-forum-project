# Build Instructions

The compiler uses CMake, and can be built with it.

LLVM 12 needs to be installed (and `llvm-config` needs to be in PATH so that CMake can find LLVM).

ANTLR v4.9.2 needs to be installed, and two options provided to CMake:

- `GALLIUM_ANTLR4_RUNTIME`: Path to the **static** ANTLR4 C++ runtime library.

- `GALLIUM_ANTLR4_RUNTIME_INCLUDE`: Path to the ANTLR4 C++ runtime's include directory

Besides those dependencies, several build flags are provided:

- `GALLIUM_TESTS`: Whether to build tests (default: `OFF`)
- `GALLIUM_BENCHES`: Whether to build benchmarks (default: `OFF`)
- `GALLIUM_DEV`: Whether to build with `-Werror` and warnings (default: `OFF`)

```bash
$ cd senior-project
$ mkdir build && cd build  
$ cmake .. -DCMAKE_BUILD_TYPE=Debug 
           -DGALLIUM_ANTLR4_COMMAND=/some/antlr4/script
           -DGALLIUM_ANTLR4_RUNTIME=/path/to/runtime.lib
```

# Documentation

Documentation can be found in `../docs/` and on the website under the
"Compiler Design" dropdown.

# Code Style

The only "unusual" part of the code style here is a simple rule: no non-`const`
references.

This means that every instance where an object could be modified when being passed into a function, a pointer (or
similar) is used, this leaves a trace at the call site (usually `&` for address-of).

Knowing this rule, consider the following code:

```cpp
std::string object = /* whatever */; 

// you can immediately see which calls could modify `object` 
do_thing(&object);
do_other_thing(object); 
```