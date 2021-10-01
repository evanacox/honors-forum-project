##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

set(GALLIUM_UTILITY_FILES utility/flags.cc )

add_executable(galliumc ./main.cc ./driver.cc ${GALLIUM_UTILITY_FILES})
configure_target(galliumc)