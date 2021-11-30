##======---------------------------------------------------------------======##
#                                                                             #
# Copyright 2021 Evan Cox <evanacox00@gmail.com>                              #
#                                                                             #
# Use of this source code is governed by a BSD-style license that can be      #
# found in the LICENSE.txt file at the root of this project, or at the        #
# following link: https://opensource.org/licenses/BSD-3-Clause                #
#                                                                             #
##======---------------------------------------------------------------======##

set(GALLIUM_AST_FILES
        ast/nodes/declaration.cc
        ast/nodes/expression.cc
        ast/nodes/statement.cc
        ast/nodes/type.cc)

set(GALLIUM_ERRORS_FILES
        errors/console_reporter.cc errors/diagnostics.cc errors/reporter.cc)

set(GALLIUM_CORE_FILES
        core/codegen.cc
        core/environment.cc
        core/typechecker.cc
        core/mangler.cc
        core/name_resolver.cc)

set(GALLIUM_UTILITY_FILES
        utility/flags.cc
        utility/log.cc
        utility/ticket_mutex.cc
        utility/pretty.cc
        utility/misc.cc)

set(GALLIUM_SYNTAX_FILES
        syntax/parser.cc
        syntax/parse_errors.cc errors/console_reporter.h errors/reporter.h errors/diagnostics.h)

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

# these are a different library so they dont get the `-Werror` added to them
# because the generated code causes warnings
add_library(gallium_antlr4 STATIC ${GALLIUM_GENERATED_FILES})
target_link_libraries(gallium_antlr4 PUBLIC ${GALLIUM_ANTLR4_RUNTIME})
target_compile_definitions(gallium_antlr4 PUBLIC ANTLR4CPP_STATIC)
target_include_directories(gallium_antlr4 SYSTEM PUBLIC
        ${GALLIUM_ANTLR4_RUNTIME_INCLUDE}
        ${GALLIUM_ANTLR4_GENERATED_DIR}/../)

add_executable(gallium
        ./main.cc
        ./driver.cc
        ${GALLIUM_AST_FILES}
        ${GALLIUM_ERRORS_FILES}
        ${GALLIUM_CORE_FILES}
        ${GALLIUM_UTILITY_FILES}
        ${GALLIUM_SYNTAX_FILES})
gallium_configure_target(gallium)
target_include_directories(gallium SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
target_compile_definitions(gallium PUBLIC ${GALLIUM_LLVM_DEFINITIONS_LIST})
target_link_directories(gallium PRIVATE ${LLVM_LIBRARY_DIRS})
target_link_libraries(gallium PRIVATE
        ${GALLIUM_LLVM_LIBS}
        gallium_antlr4
        absl::flags
        absl::flags_parse
        absl::strings
        absl::flat_hash_map)
