# Build Instructions
The compiler uses CMake, and can be built with it. 

LLVM 12 needs to be installed (and `llvm-config` needs to be in PATH
so that CMake can find LLVM). 

ANTLR v4.9.2 needs to be installed, and two options provided to CMake:

- `GALLIUM_ANTLR4_COMMAND`: A standalone command (probably a script) 
                            that has the same CLI interface as 
                            `antlr-4.9-complete.jar`. 

- `GALLIUM_ANTLR4_RUNTIME`: Path to the **static** ANTLR4 C++ runtime library. 

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