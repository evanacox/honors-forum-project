##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

add_library(gallium_runtime STATIC
        src/entry.cc
        src/runtime.cc
        src/gallium_stdlib.cc)

gallium_configure_target(gallium_runtime OFF)

if (GALLIUM_DEV)
    add_subdirectory(tests)
endif ()