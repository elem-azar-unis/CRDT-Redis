//
// Created by user on 18-11-14.
//

#ifndef BENCH_RPQ_GENERATOR_H
#define BENCH_RPQ_GENERATOR_H

#include "../constants.h"
#include "../util.h"
#include "rpq_basics.h"
#include "rpq_cmd.h"
#include "rpq_log.h"

class rpq_generator : public generator<int>
{
private:
    rpq_op_gen_pattern *pattern;
    record_for_collision add, rem;
    rpq_log &ele;
    rpq_type zt;

    static int gen_element()
    {
        return intRand(MAX_ELE);
    }

    static double gen_initial()
    {
        return doubleRand(0, MAX_INIT);
    }

    static double gen_increament()
    {
        return doubleRand(-MAX_INCR, MAX_INCR);
    }

public:
    rpq_generator(rpq_type zt, rpq_log &e, const char *p) : zt(zt), ele(e)
    {
        add_record(add);
        add_record(rem);
        start_maintaining_records();

        if (p == nullptr)
            pattern = &rpq_pt_dft;
        else if (strcmp(p, "ardominant") == 0)
            pattern = &rpq_pt_ard;
    }

    void gen_and_exec(redis_client &c) override;
};

#endif //BENCH_RPQ_GENERATOR_H
