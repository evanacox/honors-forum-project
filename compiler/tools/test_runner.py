#!/usr/bin/env python3

# ======---------------------------------------------------------------====== #
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
# ======---------------------------------------------------------------====== #

import sys
import subprocess
import tempfile
import os
from typing import List

project_root = ""
current_type = ""


class TestRunner:
    def __init__(self, mode, args, output, code):
        self.args = args
        self.mode = mode
        self.output = output
        self.code = code

    def run(self) -> List[str]:
        raise NotImplementedError()


class CheckErrorsTest(TestRunner):
    def run(self) -> List[str]:
        failures = []

        for error in self.args:
            code = error.zfill(4)

            if f"E#{code}" not in self.output:
                failures.append(f"missing error E#{code} in mode {self.mode}")

        return failures


def compile_file(tempdir, count, path, args):
    out = os.path.join(tempdir, f"output-{count}.exe")
    compiler = os.path.join(project_root, f"/build/clion/{current_type}/src/gallium")

    return subprocess.run([compiler, "--emit", "exe", "--out", out, path, *args],
                          shell=True, capture_output=True).stdout.decode("UTF-8")


def read_tests(path):
    compile_tests = []

    with open(path, "r") as f:
        tests = f.readline().split("// test: ")[1].split(", ")

        for test in tests:
            parts = f.readline()[2:].split(": ")
            name = parts[0]
            args = parts[1].split(", ")

            test_type = None

            match name:
                case "should-fail":
                    test_type = CheckErrorsTest
                case ""

            compile_tests.append([test, line])

    return compile_tests


def check_file(tempdir, count, path):
    compile_tests = read_tests(path)
    options = [[], ["--opt", "some"], ["--opt", "some"], ["--opt", "fast"]]
    success = [True] * len(options)

    for i, pair in enumerate(options):
        tests = read_tests(path)
        output = compile_file(tempdir, count, path, pair)

        for tests in output:


def main(args):
    dir = tempfile.TemporaryDirectory()

    pass


if __name__ == "__main__":
    main(sys.argv)
