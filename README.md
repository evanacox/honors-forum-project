# Senior Project
This is my high school senior project, it is purely an academic
exercise. The point is to improve my own understanding and to
create something that others can learn from, **not** to create
a completely production-grade compiler. Please do not come into
this expecting something production-grade. 

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