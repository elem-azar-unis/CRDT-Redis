cmake_minimum_required(VERSION 3.10)
project(bench)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lhiredis")

aux_source_directory(. SRC_LIST)
aux_source_directory(rpq SRC_LIST)
aux_source_directory(list SRC_LIST)

add_executable(bench_start ${SRC_LIST})

target_link_libraries(bench_start hiredis)
