# Senior Project
This is my high school senior project, it is purely an academic
exercise. The point is to improve my own understanding and to
create something that others can learn from, **not** to create
a completely production-grade compiler. Please do not come into
this expecting something production-grade. 

# Dependencies
## Website
- Jekyll (v4.2.0)
    - Used to build the `.md` files into a nice looking website

## Compiler
- LLVM (v12.0.1)
    - The compiler is an LLVM front-end, it uses the LLVM C++ API
      to generate LLVM IR thats then fed into LLVM optimizers and
      LLVM backends. 

- ANTLR (v4.9.2)
    - ANTLR is used for parser generation 

- Python (v3.9.7) 
    - Used for several tools/scripts during build 

- Google Test (v1.11.0)
    - Unit tests use Google Test

- Google Benchmark (v1.6.0)
    - Benchmarks use Google Benchmark

# Build Instructions
## Compiler
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

# License
Everything in this repository is licensed under the [BSD-3 License](./LICENSE.txt) 
located in the `LICENSE.txt` file at the root of this project. 