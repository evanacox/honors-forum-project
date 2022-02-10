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
import shutil
import os
from multiprocessing import Pool
from typing import List

project_root = ""
current_type = "debug"


def compile_file(out: str, path: str, args: List[str]) -> str:
    compiler = os.path.join(project_root, f"../build/clion/{current_type}/compiler/src/gallium.exe")

    output = subprocess.run([os.path.abspath(compiler), "--emit", "exe", "--out", out, path, *args], shell=True,
                            capture_output=True)

    return output.stdout.decode("UTF-8")


def read_test(path: str) -> (str, List[str]):
    with open(path, "r") as f:
        lines = list(map(lambda line: line.strip(), f.readlines()))
        test = lines[0].split("// test: ")[1].strip()
        args = []

        match test:
            case "should-run":
                args.append(lines[1].split("// returns: ")[1])
                args.append(lines[2].split("// outputs: ")[1])
            case "should-fail-compile":
                assert False
            case "should-panic":
                args.append(lines[1].split("// reason: ")[1])
            case "should-assert":
                args.append(lines[1].split("// reason: ")[1])
            case _:
                print(f"unknown test type '{test}'")
                assert False

        return test, args


def execute_should_run(exe: str, args: List[str]) -> str | None:
    run_output = subprocess.run([exe], shell=True, capture_output=True)
    output = run_output.stdout.decode("UTF-8")

    if int(args[0]) != run_output.returncode:
        return f"return code did not match! expected `{args[0]}` but got `{run_output.returncode}`"
    elif args[1] != "none" and args[1] != output:
        return f"output did not match! expected `{args[1]}` but got `{output}`"
    else:
        return None


def find_reason(output: str) -> str | None:
    try:
        panic_begin = output.index("gallium: panicked!")
        message_begin = output.index("  message: '", panic_begin)
        begin_quote = output.index("'", message_begin)
        end_quote = output.index("'", begin_quote + 1)

        return output[begin_quote + 1:end_quote]
    except ValueError:
        return None


def execute_should_panic(exe: str, args: List[str]) -> str | None:
    run_output = subprocess.run([exe], shell=True, capture_output=True)
    output = run_output.stderr.decode("UTF-8")
    reason = find_reason(output)

    if reason is None:
        return "expected a panic, but did not get one!"
    elif reason != args[0]:
        return f"expected reason `{args[0]}`, but got reason `{reason}`"
    else:
        return None


def execute_test(tup: (str, str, List[str])) -> str | None:
    file, test, args = tup
    out = os.path.abspath(os.path.join(os.path.curdir, "__tmp/", file.replace("/", "_").replace("\\", "_")))
    compile_output = compile_file(out, file, ["--opt", "none"])

    match test:
        case "should-run":
            if len(compile_output.strip()) != 0:
                return f"error from compiler!"

            return execute_should_run(out, args)
        case "should-panic":
            if len(compile_output.strip()) != 0:
                return f"error from compiler!"

            return execute_should_panic(out, args)
        case _:
            assert False


def parallel_execute(tests: (str, str, List[str])) -> List[str | None]:
    with Pool() as p:
        return p.map(execute_test, tests)


def main(args):
    opt_args = ["none", "some", "small", "fast"]
    tests = []

    path = os.path.join(os.curdir, "__tmp/")
    os.mkdir(path)

    try:
        for root, folders, files in os.walk("./tests/compiler/"):
            for file in files:
                test, args = read_test(os.path.join(root, file))
                tests.append((os.path.join(root, file), test, args))

        failures = []

        for i, result in enumerate(parallel_execute(tests)):
            test_path = os.path.relpath(tests[i][0], "./tests/compiler/")

            if result is None:
                print(f"\u001b[32m[passed!]\u001b[37m test {test_path}")
            else:
                print(f"\u001b[31m[failed!]\u001b[37m test {test_path}")
                failures.append((i, result))

        if len(failures) != 0:
            for i, result in failures:
                test_path = os.path.relpath(tests[i][0], "./tests/compiler/")
                print(f"\u001b[31mfailure\u001b[37m: {test_path}")
                print(f"    \u001b[34mtest type\u001b[37m: '{tests[i][1]}'")
                print(f"    \u001b[34mreason\u001b[37m: {result}")
    except:
        shutil.rmtree(path)
        raise
    
    shutil.rmtree(path)


if __name__ == "__main__":
    main(sys.argv)
