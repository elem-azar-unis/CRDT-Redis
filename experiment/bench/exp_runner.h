//
// Created by admin on 2020/1/7.
//

#ifndef BENCH_EXP_RUNNER_H
#define BENCH_EXP_RUNNER_H

#include <ctime>
#include <thread>

#include "util.h"
#include "constants.h"

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include "../../redis-4.0.8/deps/hiredis/hiredis.h"
#include <direct.h>

#endif

using namespace std;
extern const char *ips[3];

class exp_runner
{
private:
    rdt_log &log;
    generator &gen;
    cmd &read_cmd = null_cmd;

private:
    cmd &ovhd_cmd = null_cmd;
    cmd &opcount_cmd = null_cmd;

    vector<thread *> thds;
    vector<task_queue *> tasks;

    void conn_one_server_timed(const char *ip, int port);

public:
    exp_runner(rdt_log &log, generator &gen, cmd &readCmd, cmd &ovhdCmd, cmd &opcountCmd)
            : opcount_cmd(opcountCmd), ovhd_cmd(ovhdCmd), read_cmd(readCmd), gen(gen), log(log) {}

    exp_runner(rdt_log &log, generator &gen) : gen(gen), log(log) {}

    void set_cmd_read(cmd &readCmd)
    {
        read_cmd = readCmd;
    }

    void set_cmd_ovhd(cmd &ovhdCmd)
    {
        ovhd_cmd = ovhdCmd;
    }

    void set_cmd_opcount(cmd &opcountCmd)
    {
        opcount_cmd = opcountCmd;
    }

    void run();
};

#endif //BENCH_EXP_RUNNER_H
