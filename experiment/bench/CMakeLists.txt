cmake_minimum_required(VERSION 3.10)
project(bench)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -lhiredis")

add_executable(bench_start main.cpp rpq/rpq_log.h rpq/rpq_generator.h rpq/rpq_log.cpp constants.h util.h exp_runner.h rpq/rpq_generator.cpp rpq/rpq_cmd.cpp rpq/rpq_cmd.h rpq/rpq_basics.h util.cpp list/list_basics.h list/list_log.cpp list/list_log.h exp_setting.h exp_env.h rpq/rpq_exp.h rpq/rpq_exp.cpp list/list_cmd.h list/list_generator.cpp list/list_generator.h list/list_exp.cpp list/list_exp.h)

target_link_libraries(bench_start hiredis)
