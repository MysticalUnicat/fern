cmake_minimum_required(VERSION 3.2)

project(fern
  VERSION 0.0.1
  DESCRIPTION "Fern programming language, heavily inspired by BQN and others"
  LANGUAGES C
)

set(FERN_BQN_PATH "" CACHE FILEPATH "path to mlochbaum/BQN")
if(${FERN_BQN_PATH} STREQUAL "")
  message(FATAL_ERROR "FERN_BQN_PATH needs to be defined")
endif()

add_executable(lemon tool/lemon.c)
add_custom_command(
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/parse.c ${CMAKE_CURRENT_BINARY_DIR}/parse.h
  COMMAND lemon -q -d${CMAKE_CURRENT_BINARY_DIR} -T${CMAKE_CURRENT_SOURCE_DIR}/tool/lempar.c ${CMAKE_CURRENT_SOURCE_DIR}/src/parse.y
  MAIN_DEPENDENCY src/parse.y
  DEPENDS lemon src/parse.y
)

set(FERN_BQN_SOURCES)
set(FERN_BQN_NAMES)

function(fern_compile_bqn module)
  set(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/bqn_${module}.c)

  set(FERN_BQN_SOURCES ${FERN_BQN_SOURCES} ${OUTPUT} PARENT_SCOPE)
  set(FERN_BQN_NAMES ${FERN_BQN_NAMES} "fern_bqn_module_${module}")

  add_custom_command(
    OUTPUT ${OUTPUT}
    COMMAND env bqn ${CMAKE_CURRENT_SOURCE_DIR}/tool/cf.bqn ${FERN_BQN_PATH} ${module} > ${OUTPUT}
    MAIN_DEPENDENCY tool/cf.bqn
  )
endfunction()

fern_compile_bqn(r0)
fern_compile_bqn(r1)
fern_compile_bqn(cc)
fern_compile_bqn(f)

list(JOIN FERN_BQN_NAMES "," FERN_BQN_NAMES)

add_library(fernrt
  src/runtime.c                       # runtime memory management
  src/internal.c                      # internal functions for implementing primitives
  src/primitives.c                    # all function, modifier1 and modifier2 primitives
  src/backend.c                       # stuff for using the backend (compile to C, running C compiler, loading resulting object)
  src/bqn.c ${FERN_BQN_SOURCES}       # interopability with BQN.
  ${CMAKE_CURRENT_BINARY_DIR}/parse.c # lexer and parser combination
)
target_include_directories(fernrt
  PUBLIC include
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_definitions(fernrt PRIVATE FERN_BQN_NAMES=$<JOIN:FERN_BQN_NAMES,$<COMMA>>)
target_compile_options(fernrt PUBLIC -Wall -Werror -std=c11)

add_executable(fern src/fern.c)

target_link_libraries(fern PUBLIC fernrt)

