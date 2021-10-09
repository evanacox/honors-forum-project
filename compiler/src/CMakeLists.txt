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

set(GALLIUM_SYNTAX_GENERATED_FILES
        syntax/generated/GalliumBaseVisitor.cpp
        syntax/generated/GalliumLexer.cpp
        syntax/generated/GalliumParser.cpp
        syntax/generated/GalliumVisitor.cpp)

add_executable(gallium ./main.cc ./driver.cc ${GALLIUM_UTILITY_FILES} ${GALLIUM_SYNTAX_FILES} ${GALLIUM_SYNTAX_GENERATED_FILES})
configure_target(gallium)

target_include_directories(gallium PUBLIC
        ${LLVM_INCLUDE_DIRS}
        ${GALLIUM_ANTLR4_RUNTIME_INCLUDE})

# need to set some defines for LLVM headers to not fail to compile when included
target_compile_definitions(gallium PUBLIC ${LLVM_DEFINITIONS_LIST})

# need to make ANTLR4 not make mingw link against dynamic symbols
target_compile_definitions(gallium PUBLIC -DANTLR4CPP_STATIC)

# link against **static** libs
target_link_libraries(gallium PRIVATE
        ${GALLIUM_LLVM_LIBS}
        ${GALLIUM_ANTLR4_RUNTIME}
        absl::flags
        absl::flags_parse
        absl::strings
        absl::flat_hash_map)