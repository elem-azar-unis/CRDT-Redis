cmake_minimum_required(VERSION 3.10)
project(dmck)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lhiredis")

aux_source_directory(. SRC_LIST)

add_executable(dmck_run ${SRC_LIST} redis_connect.h)

target_link_libraries(dmck_run hiredis)
