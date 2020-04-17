//
// Created by user on 18-11-15.
//

#ifndef BENCH_CONSTANTS_H
#define BENCH_CONSTANTS_H

#include "exp_setting.h"

// experiment settings
//#define DELAY 50
//#define DELAY_LOW 10
//#define TOTAL_SERVERS 9
//#define OP_PER_SEC 10000

#define MAX_TIME_COLISION exp_setting::delay
#define SPLIT_NUM 10
#define SLP_TIME_MICRO (MAX_TIME_COLISION * 1000 / SPLIT_NUM)

#define THREAD_PER_SERVER 3
#define TOTAL_SERVERS (exp_setting::total_clusters * exp_setting::server_per_cluster)
#define OP_PER_THREAD (exp_setting::total_ops/TOTAL_SERVERS/THREAD_PER_SERVER)

// time in microseconds
#define INTERVAL_TIME (1000000.0*TOTAL_SERVERS*THREAD_PER_SERVER/exp_setting::op_per_sec)
//#define OP_PS_PTH ((double)OP_PER_SEC/TOTAL_SERVERS/THREAD_PER_SERVER)
//#define ESTIMATE_COST (TOTAL_SERVERS*THREAD_PER_SERVER*(26.3629 + 0.0082*OP_PS_PTH))
//#define _SLEEP_TIME (INTERVAL_TIME-ESTIMATE_COST)
//#define SLEEP_TIME ((_SLEEP_TIME>0)?_SLEEP_TIME:0)

// time in seconds
#define TIME_OVERHEAD 1
#define TIME_MAX 1

#endif //BENCH_CONSTANTS_H
