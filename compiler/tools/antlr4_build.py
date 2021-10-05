#!/usr/bin/env python3

##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

import re
import os


def tools_dir():
    return os.path.dirname(os.path.realpath(__file__))


def syntax_dir():
    return tools_dir() + "/../src/syntax/"


def main():
    syntax = syntax_dir()

    os.system(f"antlr4 -Dlanguage=Cpp -no-listener -visitor -o {syntax}/generated {syntax}/Gallium.g4")

    for file in os.listdir(f"{syntax}/generated"):
        if file.endswith(".interp") or file.endswith(".tokens"):
            os.remove(os.path.join(f"{syntax}/generated/", file))

    # may add more logic in the future


if __name__ == "__main__":
    main()
