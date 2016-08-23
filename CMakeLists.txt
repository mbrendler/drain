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
)
target_link_libraries(drain termcap)