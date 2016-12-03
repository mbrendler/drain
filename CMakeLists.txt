cmake_minimum_required(VERSION 2.8.0)
project("drain")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wpedantic")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-gnu-zero-variadic-macro-arguments")
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${CMAKE_C_FLAGS} -O0 -g")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${CMAKE_C_FLAGS} -O3")

set(SRCS
  src/config.c
  src/drainfile.c
  src/process_list.c
  src/process.c
  src/server.c
  src/client.c
  src/actions.c
  src/commands.c
  src/cmd_server.c
  src/cmd_attach.c
  src/helpers.c
)

add_library(_drain ${SRCS})

add_executable(drain src/drain.c)
target_link_libraries(drain _drain termcap)

add_executable(test_helpers tests/test_helpers.c src/helpers.c)

add_executable(test_config tests/test_config.c)
target_link_libraries(test_config _drain termcap)

add_executable(test_drainfile tests/test_drainfile.c)
target_link_libraries(test_drainfile _drain termcap)

add_executable(test_server_client tests/test_server_client.c src/server.c src/client.c src/config.c)
target_link_libraries(test_server_client termcap)

add_executable(test_actions tests/test_actions.c src/actions.c src/process_list.c src/helpers.c src/config.c)
target_link_libraries(test_actions termcap)

enable_testing()

add_test(test_helpers test_helpers)
add_test(test_config test_config)
add_test(test_drainfile test_drainfile)
add_test(test_server_client test_server_client)
add_test(test_actions test_actions)
