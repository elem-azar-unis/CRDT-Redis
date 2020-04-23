//
// Created by admin on 2020/1/8.
//

#ifndef BENCH_RPQ_BASICS_H
#define BENCH_RPQ_BASICS_H

#define MAX_ELE 200000
#define MAX_INIT 100
#define MAX_INCR 50

#define PR_ADD 0.41
#define PR_INC 0.2
#define PR_REM 0.39

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

enum class rpq_op_type
{
    add = 0, incrby = 1, rem = 2, max = 3, overhead = 4, opcount = 5
};

extern const char *rpq_cmd_prefix[2];

enum class rpq_type
{
    o = 0, r = 1
};

#endif //BENCH_RPQ_BASICS_H
