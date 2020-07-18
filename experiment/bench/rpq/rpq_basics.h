//
// Created by admin on 2020/1/8.
//

#ifndef BENCH_RPQ_BASICS_H
#define BENCH_RPQ_BASICS_H

constexpr int MAX_ELE = 200000;
constexpr int MAX_INIT = 100;
constexpr int MAX_INCR = 50;

struct rpq_op_gen_pattern
{
    double PR_ADD;
    double PR_INC;
    double PR_REM;

    double PR_ADD_CA;
    double PR_ADD_CR;
    double PR_REM_CA;
    double PR_REM_CR;
};

extern rpq_op_gen_pattern rpq_pt_dft, rpq_pt_ard;

enum class rpq_op_type
{
    add,
    incrby,
    rem,
    max,
    overhead,
    opcount
};

#define RPQ_TYPE_CODEC(ACTION) \
    ACTION(o)                  \
    ACTION(r)                  \
    ACTION(rwf)

#define DEFINE_ACTION(_name) _name,
enum class rpq_type
{
    RPQ_TYPE_CODEC(DEFINE_ACTION)
};
#undef DEFINE_ACTION

extern const char *rpq_type_str[];

#endif  // BENCH_RPQ_BASICS_H
