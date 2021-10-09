##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

cmake_minimum_required(VERSION 3.15)
project(gallium)

set(CMAKE_CXX_STANDARD 17) # need to affect libraries as well, not just gallium

find_package(LLVM REQUIRED)
find_package(Python3 COMPONENTS Interpreter REQUIRED)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

llvm_map_components_to_libnames(GALLIUM_LLVM_LIBS support core irreader)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

set(ABSL_DEFAULT_COPTS)
add_subdirectory(vendor/abseil-cpp)

option(GALLIUM_DEV "whether or not to build with `-Werror` or the compiler's equivalent" OFF)

function(gallium_configure_target TARGET)
    list(APPEND GALLIUM_WARNINGS)

    if (CMAKE_CXX_COMPILER EQ "MSVC")
        list(APPEND GALLIUM_WARNINGS /W3)
    endif ()

    # enable warnings and warnings-as-errors in development
    # there's no reason not to, and its off by default for normal use
    if (GALLIUM_DEV)
        target_compile_options(${TARGET} PRIVATE ${GALLIUM_WARNINGS})
    endif ()
endfunction()

add_subdirectory(src)