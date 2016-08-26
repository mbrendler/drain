cmake_minimum_required(VERSION 2.8.0)
project("drain")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic -O0 -g")

add_executable(drain
  src/drain.c
  src/config.c
  src/process_list.c
  src/process.c
  src/server.c
  src/client.c
  src/actions.c
  src/helpers.c
)
target_link_libraries(drain termcap)

add_executable(test_helpers tests/test_helpers.c src/helpers.c)
enable_testing()

add_test(test_helpers test_helpers)
