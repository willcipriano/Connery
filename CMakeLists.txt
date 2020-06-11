cmake_minimum_required(VERSION 3.16)
project(Connery C)

set(CMAKE_C_STANDARD 99)

add_executable(Connery main.c)
target_link_libraries(Connery PRIVATE /usr/lib/x86_64-linux-gnu/libedit.so)
