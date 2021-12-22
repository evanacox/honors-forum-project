##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

FetchContent_MakeAvailable(googletest)

set(GALLIUM_RUNTIME_UNIT_TESTS
        )

# add_executable(gallium_runtime_tests ${GALLIUM_RUNTIME_UNIT_TESTS})
# target_link_libraries(gallium_runtime_tests PRIVATE gtest_main)
# target_include_directories(gallium_runtime_tests PRIVATE "../")
# gallium_configure_target(gallium_runtime_tests