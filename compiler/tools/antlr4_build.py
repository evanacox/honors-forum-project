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

import os
import sys


def output_dir():
    return sys.argv[1]


def syntax_dir():
    return os.path.dirname(os.path.realpath(__file__)) + "/../src/syntax/"


def main():
    syntax = syntax_dir()
    out = output_dir()

    os.system(f"antlr4 -Dlanguage=Cpp -no-listener -visitor -o {out} {syntax}/Gallium.g4")

    for file in os.listdir(out):
        if file.endswith(".interp") or file.endswith(".tokens"):
            os.remove(os.path.join(out, file))

    # may add more logic in the future


if __name__ == "__main__":
    main()
