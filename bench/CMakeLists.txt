cmake_minimum_required(VERSION 3.10)
project(bench C)

set(CMAKE_C_STANDARD 99)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -lhiredis")

add_executable(bench_start main.c)

target_link_libraries(bench_start hiredis)
