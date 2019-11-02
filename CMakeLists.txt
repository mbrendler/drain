cmake_minimum_required(VERSION 3.9.0)
project("drain" C)

include("${CMAKE_SOURCE_DIR}/cmake/cotire.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/cppcheck.cmake")
include("${CMAKE_SOURCE_DIR}/cmake/clang-tidy.cmake")

set_property(GLOBAL PROPERTY COTIRE_ENABLE_PRECOMPILED_HEADER FALSE)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
  -Wall \
  -Wextra \
  -Wno-gnu-zero-variadic-macro-arguments \
")

if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -Wpedantic \
    -Weverything \
    -Wno-padded \
    -Wno-missing-prototypes \
  ")
elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")
endif()

set(IPO_SUPPORT FALSE)
if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT IPO_SUPPORT OUTPUT output)
endif()

find_library(TERMCAP_LIB NAMES termcap ncurses)

add_library(_drain EXCLUDE_FROM_ALL OBJECT
  src/config.c
  src/drainfile.c
  src/process_list.c
  src/process.c
  src/server.c
  src/client.c
  src/actions.c
  src/error.c
  src/commands.c
  src/cmd_server.c
  src/cmd_attach.c
  src/helpers.c
)
cotire(_drain)

add_executable(drain EXCLUDE_FROM_ALL src/drain.c $<TARGET_OBJECTS:_drain>)
set_property(TARGET drain PROPERTY INTERPROCEDURAL_OPTIMIZATION ${IPO_SUPPORT})
target_link_libraries(drain "${TERMCAP_LIB}")
cotire(drain)

add_executable(test_helpers EXCLUDE_FROM_ALL tests/test_helpers.c src/helpers.c)
cotire(test_helpers)

add_executable(test_config EXCLUDE_FROM_ALL
  tests/test_config.c
  $<TARGET_OBJECTS:_drain>
)
target_link_libraries(test_config "${TERMCAP_LIB}")
cotire(test_config)

add_executable(test_drainfile EXCLUDE_FROM_ALL
  tests/test_drainfile.c
  $<TARGET_OBJECTS:_drain>
)
target_link_libraries(test_drainfile "${TERMCAP_LIB}")
cotire(test_drainfile)

add_executable(test_server_client EXCLUDE_FROM_ALL
  tests/test_server_client.c
  src/server.c
  src/client.c
  src/config.c
)
target_link_libraries(test_server_client "${TERMCAP_LIB}")
cotire(test_server_client)

add_executable(test_actions EXCLUDE_FROM_ALL
  tests/test_actions.c
  src/actions.c
  tests/process_mock.c
  src/process_list.c
  src/helpers.c
  src/config.c
)
target_link_libraries(test_actions "${TERMCAP_LIB}")
cotire(test_actions)

add_executable(test_process_list EXCLUDE_FROM_ALL
  tests/test_process_list.c
  tests/process_mock.c
  src/process_list.c
)
cotire(test_process_list)

add_executable(test_process EXCLUDE_FROM_ALL
  tests/test_process.c
  $<TARGET_OBJECTS:_drain>
)
target_link_libraries(test_process "${TERMCAP_LIB}")
cotire(test_process)

add_custom_target(build_all ALL DEPENDS all_unity)

enable_testing()

add_test(test_helpers test_helpers)
add_test(test_config test_config)
add_test(test_drainfile test_drainfile)
add_test(test_server_client test_server_client)
add_test(test_actions test_actions)
add_test(test_process_list test_process_list)
add_test(test_process test_process)

add_test(test_system
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_system.sh"
  "${CMAKE_BINARY_DIR}"
)
