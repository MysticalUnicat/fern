cmake_minimum_required(VERSION 3.2)

project(fern
  VERSION 0.0.1
  DESCRIPTION "Fern programming language, heavily inspired by BQN and others"
  LANGUAGES C
)

add_executable(lemon tool/lemon.c)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parse.c ${CMAKE_CURRENT_BINARY_DIR}/parse.h
  COMMAND lemon -q -d${CMAKE_CURRENT_BINARY_DIR} -T${CMAKE_CURRENT_SOURCE_DIR}/tool/lempar.c ${CMAKE_CURRENT_SOURCE_DIR}/src/parse.y
  MAIN_DEPENDENCY src/parse.y
  DEPENDS lemon src/parse.y
)

add_library(fernrt
  src/runtime.c                       # runtime memory management
  src/internal.c                      # internal functions for implementing primitives
  src/primitives.c                    # all function, modifier1 and modifier2 primitives
  src/backend.c                       # stuff for using the backend (compile to C, running C compiler, loading resulting object)
  src/bqn.c                           # interopability with BQN. in particular a bytecode interpreter and transpiler to backend
  ${CMAKE_CURRENT_BINARY_DIR}/parse.c # lexer and parser combination
)
target_include_directories(fernrt
  PUBLIC include
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
)
target_compile_options(fernrt PUBLIC -Wall -Werror -std=c11)

add_executable(fern src/fern.c)

target_link_libraries(fern PUBLIC fernrt)

