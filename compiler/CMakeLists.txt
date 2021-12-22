##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

find_package(LLVM REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

llvm_map_components_to_libnames(GALLIUM_LLVM_LIBS support core bitreader AllTargetsCodeGens AllTargetsAsmParsers AllTargetsDescs AllTargetsDisassemblers AllTargetsInfos)
separate_arguments(GALLIUM_LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

message(STATUS "Found LLVM libraries ${GALLIUM_LLVM_LIBS}")

add_subdirectory(vendor/abseil-cpp)

add_subdirectory(src)

if (GALLIUM_DEV)
    add_subdirectory(tests)
endif ()