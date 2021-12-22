# Senior Project
This is my high school senior project, it is purely an academic
exercise. The point is to improve my own understanding and to
create something that others can learn from, **not** to create
a completely production-grade compiler. Please do not come into
this expecting something production-grade. 

# Build Instructions
The entire project uses CMake, and can be built with it.

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
Documentation can be found in `./docs/` and on the website under the
"Compiler Design" dropdown.

# Dependencies
## Website
- Jekyll (v3.9.0)
    - Used to build the `.md` files into a nice looking website

- Just The Docs (v0.3.3)
    - Nice theme for the aforementioned Jekyll site

## Compiler
- LLVM (v12.0.1)
    - The compiler is an LLVM front-end, it uses the LLVM C++ API
      to generate LLVM IR thats then fed into LLVM optimizers and
      LLVM backends. 

- Google Abseil 
    - Utility library with a large number of helpful and 
      very fast types/functions used throughout the compiler

- ANTLR (v4.9.2)
    - ANTLR is used for parser generation 

- Python (v3.9.7) 
    - Used for several tools/scripts during build 

- Google Test (v1.11.0)
    - Unit tests use Google Test

- Google Benchmark (v1.6.0)
    - Benchmarks use Google Benchmark

# License
Everything in this repository is licensed under the [BSD-3 License](./LICENSE.txt) 
located in the `LICENSE.txt` file at the root of this project. If for whatever
reason that file is missing, it can be found [here](https://opensource.org/licenses/BSD-3-Clause).