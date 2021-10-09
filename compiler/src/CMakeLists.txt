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

set(GALLIUM_ANTLR4_GENERATED_DIR ${CMAKE_BINARY_DIR}/antlr4/generated)

set(GALLIUM_GENERATED_FILES
        ${GALLIUM_ANTLR4_GENERATED_DIR}/GalliumBaseVisitor.cpp
        ${GALLIUM_ANTLR4_GENERATED_DIR}/GalliumLexer.cpp
        ${GALLIUM_ANTLR4_GENERATED_DIR}/GalliumParser.cpp
        ${GALLIUM_ANTLR4_GENERATED_DIR}/GalliumVisitor.cpp)

add_custom_command(OUTPUT ${GALLIUM_GENERATED_FILES}
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/../tools/antlr4_build.py ${GALLIUM_ANTLR4_GENERATED_DIR}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/syntax/Gallium.g4
        COMMENT "running antlr4_build.py on Gallium.g4")

add_executable(gallium ./main.cc ./driver.cc ${GALLIUM_UTILITY_FILES} ${GALLIUM_SYNTAX_FILES} ${GALLIUM_GENERATED_FILES})
target_include_directories(gallium PUBLIC ${LLVM_INCLUDE_DIRS})

# make include directories for ANTLR4 system, because they don't work in `-Werror`
target_include_directories(gallium SYSTEM PUBLIC
        ${GALLIUM_ANTLR4_RUNTIME_INCLUDE}
        ${GALLIUM_ANTLR4_GENERATED_DIR}/../)

# need to set some defines for LLVM headers to not fail to compile when included
target_compile_definitions(gallium PUBLIC ${LLVM_DEFINITIONS_LIST})

# need to make ANTLR4 not make mingw link against dynamic symbols
target_compile_definitions(gallium PUBLIC ANTLR4CPP_STATIC)

# link against **static** libs
target_link_libraries(gallium PRIVATE
        ${GALLIUM_LLVM_LIBS}
        ${GALLIUM_ANTLR4_RUNTIME}
        absl::flags
        absl::flags_parse
        absl::strings
        absl::flat_hash_map)

gallium_configure_target(gallium)
