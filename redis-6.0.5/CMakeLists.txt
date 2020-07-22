cmake_minimum_required(VERSION 3.9)
project(redis_6_0_5)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_C_STANDARD 11)

include_directories(deps/hiredis deps/linenoise deps/lua/src)

aux_source_directory(src SRC_LIST)

add_executable(redis_6_0_5 ${SRC_LIST})
