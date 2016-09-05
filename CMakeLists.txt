cmake_minimum_required(VERSION 2.8.0)
project("drain")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_FLAGS} -O0 -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${CMAKE_C_FLAGS} -O3")

set(SRCS
  src/config.c
  src/process_list.c
  src/process.c
  src/server.c
  src/client.c
  src/actions.c
  src/commands.c
  src/cmd_server.c
  src/helpers.c
)

add_executable(drain ${SRCS} src/drain.c)
target_link_libraries(drain termcap)

add_executable(test_helpers tests/test_helpers.c src/helpers.c)
add_executable(test_config tests/test_config.c ${SRCS})
target_link_libraries(test_config termcap)
enable_testing()

add_test(test_helpers test_helpers)
add_test(test_config test_config)
