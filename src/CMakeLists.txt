cmake_minimum_required(VERSION 3.13)
project(Connery C)

set(CMAKE_C_STANDARD 99)
set(CURL_LIBRARY "-lcurl")
find_package(CURL REQUIRED)

add_executable(Connery main.c mpc.c util.c util.h hashtable.c hashtable.h cval.h cval.c trace.c trace.h strings.c strings.h allocator.c allocator.h http.c http.h globals.h json_parser.c json_parser.h murmurhash.c murmurhash.h)
include_directories(${CURL_INCLUDE_DIR})	
target_link_libraries(Connery PRIVATE ${CURL_LIBRARIES} json-c)

add_custom_command(
        TARGET Connery PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/stdlib
        ${CMAKE_CURRENT_BINARY_DIR}/stdlib)
