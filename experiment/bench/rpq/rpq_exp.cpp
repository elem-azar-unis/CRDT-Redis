//
// Created by admin on 2020/6/30.
//

#include "rpq_exp.h"

#include "../exp_runner.h"
#include "rpq_generator.h"

exp_setting::default_setting rpq_exp::rpq_setting{
    .total_sec = 2000,
    .delay = 50,
    .delay_low = 10,
    .total_clusters = 3,
    .server_per_cluster = 3,
    .op_per_sec = 10000,
    .speed_e = {.start = 500, .end = 10000, .step = 100},
    .replica_e = {.start = 1, .end = 5, .step = 1},
    .delay_e = {.start = 20, .end = 380, .step = 40}};

void rpq_exp::exp_impl(const string& type, const string& pattern)
{
    rpq_log qlog(type);
    rpq_generator gen(type, qlog, pattern);
    rpq_max_cmd read_max(type, qlog);
    rpq_overhead_cmd ovhd(type, qlog);

    exp_runner runner(qlog, gen);
    runner.set_cmd_ovhd(ovhd);
    runner.set_cmd_read(read_max);
    runner.run();
}
