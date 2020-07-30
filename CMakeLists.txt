cmake_minimum_required(VERSION 3.16)
project(Connery C)

set(CMAKE_C_STANDARD 99)

add_executable(Connery main.c mpc.c)
target_link_libraries(Connery PRIVATE /usr/lib/x86_64-linux-gnu/libedit.so)

add_custom_command(
        TARGET Connery PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/stdlib
        ${CMAKE_CURRENT_BINARY_DIR}/stdlib)
