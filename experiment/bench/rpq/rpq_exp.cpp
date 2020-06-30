//
// Created by admin on 2020/6/30.
//

#include "rpq_exp.h"
#include "rpq_generator.h"
#include "../exp_runner.h"

exp_setting::default_setting rpq_exp::rpq_setting{
    .delay = 50,
    .delay_low = 10,
    .total_clusters = 3,
    .server_per_cluster = 3,
    .op_per_sec = 10000,
    .speed_e{.start = 500, .end = 10000, .step = 100},
    .replica_e{.start = 1, .end = 5, .step = 1},
    .delay_e{.start = 20, .end = 380, .step = 1}};

void rpq_exp::exp_impl(rpq_type type, const char *pattern)
{
    rpq_log qlog(type);
    rpq_generator gen(type, qlog, pattern);
    rpq_cmd read_max(type, rpq_op_type::max, -1, -1, qlog);
    rpq_cmd ovhd(type, rpq_op_type::overhead, -1, -1, qlog);
    rpq_cmd opcount(type, rpq_op_type::opcount, -1, -1, qlog);

    exp_runner<int> runner(qlog, gen);
    runner.set_cmd_opcount(opcount);
    runner.set_cmd_ovhd(ovhd);
    runner.set_cmd_read(read_max);
    runner.run();
}
