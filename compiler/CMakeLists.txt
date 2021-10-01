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

find_package(LLVM REQUIRED)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

llvm_map_components_to_libnames(GALLIUM_LLVM_LIBS support core irreader)
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})

function(configure_target TARGET)
    set_target_properties(${TARGET} PROPERTIES
            CXX_STANDARD 20
            CXX_EXTENSIONS OFF)
    target_include_directories(${TARGET} PUBLIC ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(${TARGET} PUBLIC ${LLVM_DEFINITIONS_LIST})
    target_link_libraries(${TARGET} PUBLIC ${GALLIUM_LLVM_LIBS})
endfunction()

add_subdirectory(src)