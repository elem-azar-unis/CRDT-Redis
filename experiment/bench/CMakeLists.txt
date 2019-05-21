cmake_minimum_required(VERSION 3.10)
project(bench)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lhiredis")

add_executable(bench_start main.cpp queue_log.h generator.h queue_log.cpp constants.h)

target_link_libraries(bench_start hiredis)
