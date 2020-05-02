//
// Created by admin on 2020/1/8.
//

#ifndef BENCH_RPQ_BASICS_H
#define BENCH_RPQ_BASICS_H

constexpr int MAX_ELE = 200000;
constexpr int MAX_INIT = 100;
constexpr int MAX_INCR = 50;

constexpr double PR_ADD = 0.41;
constexpr double PR_INC = 0.2;
constexpr double PR_REM = 0.39;

constexpr double PR_ADD_CA = 0.15;
constexpr double PR_ADD_CR = 0.05;
constexpr double PR_REM_CA = 0.1;
constexpr double PR_REM_CR = 0.1;

constexpr double PA = PR_ADD;
constexpr double PI = PR_ADD + PR_INC;
constexpr double PAA = PR_ADD_CA;
constexpr double PAR = PR_ADD_CA + PR_ADD_CR;
constexpr double PRA = PR_REM_CA;
constexpr double PRR = PR_REM_CA + PR_REM_CR;

enum class rpq_op_type
{
    add = 0, incrby = 1, rem = 2, max = 3, overhead = 4, opcount = 5
};

#define RPQ_TYPE_CODEC(ACTION)      \
    ACTION(o)                       \
    ACTION(r)                       \
    ACTION(rwf)

#define DEFINE_ACTION(_name) _name,
enum class rpq_type
{
    RPQ_TYPE_CODEC(DEFINE_ACTION)
};
#undef DEFINE_ACTION

extern const char *rpq_cmd_prefix[2];

#endif //BENCH_RPQ_BASICS_H
