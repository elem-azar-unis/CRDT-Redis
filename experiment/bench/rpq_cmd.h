//
// Created by admin on 2020/1/8.
//

#ifndef BENCH_RPQ_CMD_H
#define BENCH_RPQ_CMD_H

#include "util.h"
#include "rpq_basics.h"
#include "rpq_log.h"

class rpq_cmd : public cmd
{
private:
    rpq_type zt;
    rpq_op_type t;
    int e;
    double d;
    rpq_log &ele;

public:

    rpq_cmd(rpq_type zt, rpq_op_type t, int e, double d, rpq_log &em) : zt(zt), t(t), e(e), d(d), ele(em) {}

    rpq_cmd(const rpq_cmd &c) = default;

    void exec(redisContext *c) override;
};


#endif //BENCH_RPQ_CMD_H
