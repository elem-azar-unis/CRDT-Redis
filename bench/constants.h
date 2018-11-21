//
// Created by user on 18-11-15.
//

#ifndef BENCH_CONSTANTS_H
#define BENCH_CONSTANTS_H

#define Z_TYPE o
#define MAX_TIME_MILI 100
#define SPLIT_NUM 10
#define SLP_TIME_MICRO (MAX_TIME_MILI * 1000 / SPLIT_NUM)

#define TOTAL_SERVERS 5
#define THREAD_PER_SERVER 3

#define TOTAL_OPS 200000
#define OP_PER_THREAD (TOTAL_OPS/TOTAL_SERVERS/THREAD_PER_SERVER)

#define OP_PER_SEC 30000
// time in microseconds
#define ESTIMATE_COST 30
#define _SLEEP_TIME (1000000/(OP_PER_SEC/TOTAL_SERVERS/THREAD_PER_SERVER)-ESTIMATE_COST)
#define SLEEP_TIME ((_SLEEP_TIME>0)?_SLEEP_TIME:0)

#define TIME_OVERHEAD 5
#define TIME_MAX 5

#define MAX_ELE 20000
#define MAX_INIT 100
#define MAX_INCR 100

#define PR_ADD 0.11
#define PR_INC 0.8
#define PR_REM 0.09

#define PR_ADD_CA 0.15
#define PR_ADD_CR 0.05
#define PR_REM_CA 0.1
#define PR_REM_CR 0.1

#define PA PR_ADD
#define PI (PR_ADD + PR_INC)
#define PAA PR_ADD_CA
#define PAR (PR_ADD_CA + PR_ADD_CR)
#define PRA PR_REM_CA
#define PRR (PR_REM_CA + PR_REM_CR)

#endif //BENCH_CONSTANTS_H
