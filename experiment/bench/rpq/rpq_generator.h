//
// Created by user on 18-11-14.
//

#ifndef BENCH_RPQ_GENERATOR_H
#define BENCH_RPQ_GENERATOR_H

#include "../util.h"
#include "rpq_cmd.h"
#include "rpq_log.h"

class rpq_generator : public generator
{
private:
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

    static constexpr int MAX_ELE = 200000;
    static constexpr int MAX_INIT = 100;
    static constexpr int MAX_INCR = 50;

    rpq_op_gen_pattern &pattern;
    record_for_collision<int> add, rem;
    rpq_log &ele;
    const string &zt;

    static rpq_op_gen_pattern &get_pattern(const string &name);
    rpq_add_cmd gen_add();

public:
    rpq_generator(const string &type, rpq_log &e, const string &p)
        : zt(type), ele(e), pattern(get_pattern(p))
    {
        add_record(add);
        add_record(rem);
        start_maintaining_records();
    }

    void gen_and_exec(redis_client &c) override;
};

#endif  // BENCH_RPQ_GENERATOR_H
