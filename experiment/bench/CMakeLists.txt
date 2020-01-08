cmake_minimum_required(VERSION 3.10)
project(bench)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lhiredis")

add_executable(bench_start main.cpp rpq_log.h rpq_generator.h rpq_log.cpp constants.h util.h exp_runner.h exp_runner.cpp rpq_generator.cpp rpq_cmd.cpp rpq_cmd.h rpq_basics.h)

target_link_libraries(bench_start hiredis)
