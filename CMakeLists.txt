##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

cmake_minimum_required(VERSION 3.17)
project(gallium)

set(CMAKE_CXX_STANDARD 17) # need to affect libraries as well, not just gallium

option(GALLIUM_DEV "whether or not to build with `-Werror` or the compiler's equivalent" OFF)

function(gallium_configure_target TARGET)
    list(APPEND GALLIUM_WARNINGS)

    if (${MSVC})
        list(APPEND GALLIUM_WARNINGS /W3)
    else ()
        list(APPEND GALLIUM_WARNINGS
                -Wall
                -Wextra
                -Wcast-qual
                -Wconversion-null
                -Wmissing-declarations
                -Woverlength-strings
                -Wpointer-arith
                -Wunused-local-typedefs
                -Wunused-result
                -Wvarargs
                -Wvla
                -Wwrite-strings
                -Werror
                -ftemplate-backtrace-limit=20)
    endif ()

    # enable warnings and warnings-as-errors in development
    # there's no reason not to, and its off by default for normal use
    if (GALLIUM_DEV)
        target_compile_options(${TARGET} PRIVATE ${GALLIUM_WARNINGS})
        message(STATUS "adding ${GALLIUM_WARNINGS} to ${TARGET}")
    endif ()

    target_compile_features(${TARGET} PUBLIC cxx_std_17)
endfunction()

add_subdirectory(compiler)
add_subdirectory(runtime)