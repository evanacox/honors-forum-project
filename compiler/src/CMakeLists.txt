##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

set(GALLIUM_UTILITY_FILES
        utility/flags.cc
        utility/log.cc
        utility/ticket_mutex.h
        utility/ticket_mutex.cc)

set(GALLIUM_SYNTAX_FILES
        syntax/parser.cc
        syntax/lexer.cc)

add_executable(gallium ./main.cc ./driver.cc ${GALLIUM_UTILITY_FILES})
configure_target(gallium)
target_include_directories(gallium PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(gallium PUBLIC ${LLVM_DEFINITIONS_LIST})
target_link_libraries(gallium PRIVATE
        ${GALLIUM_LLVM_LIBS}
        absl::flags
        absl::flags_parse
        absl::strings
        absl::flat_hash_map)