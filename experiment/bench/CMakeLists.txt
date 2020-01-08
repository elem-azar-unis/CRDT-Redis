cmake_minimum_required(VERSION 3.10)
project(bench)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lhiredis")

add_executable(bench_start main.cpp rpq/rpq_log.h rpq/rpq_generator.h rpq/rpq_log.cpp constants.h util.h exp_runner.h exp_runner.cpp rpq/rpq_generator.cpp rpq/rpq_cmd.cpp rpq/rpq_cmd.h rpq/rpq_basics.h util.cpp)

target_link_libraries(bench_start hiredis)
