//
// Created by admin on 2020/1/7.
//

#ifndef BENCH_EXP_RUNNER_H
#define BENCH_EXP_RUNNER_H

#include <future>
#include <thread>

#include "exp_env.h"
#include "exp_setting.h"
#include "util.h"

#if defined(__linux__)

#include <hiredis/hiredis.h>

#elif defined(_WIN32)

#include <direct.h>

#include "../../redis-6.0.5/deps/hiredis/hiredis.h"

#endif

constexpr int THREAD_PER_SERVER = 1;
#define RUN_CONDITION log.write_op_executed < exp_setting::total_ops

// time in seconds
#define INTERVAL_TIME ((double)TOTAL_SERVERS * THREAD_PER_SERVER / exp_setting::op_per_sec)
constexpr int TIME_OVERHEAD = 1;
constexpr int TIME_READ = 1;

using namespace std;
// extern const char *ips[];

class exp_runner
{
private:
    rdt_log &log;
    generator &gen;
    cmd *read_cmd = nullptr;
    cmd *ovhd_cmd = nullptr;

    vector<thread> thds;

    void conn_one_server_timed(const char *ip, int port)
    {
        for (int i = 0; i < THREAD_PER_SERVER; ++i)
        {
            thds.emplace_back([this, ip, port] {
                redis_client c(ip, port);
                auto start_time = chrono::steady_clock::now();
                for (int t = 1; RUN_CONDITION; ++t)
                {
                    gen.gen_and_exec(c);
                    auto tar_time = start_time + chrono::duration<double>(t * INTERVAL_TIME);
                    this_thread::sleep_until(tar_time);
                }
            });
        }
    }

public:
    exp_runner(rdt_log &log, generator &gen) : gen(gen), log(log) {}

    void set_cmd_read(cmd &readCmd) { read_cmd = &readCmd; }

    void set_cmd_ovhd(cmd &ovhdCmd) { ovhd_cmd = &ovhdCmd; }

    void run()
    {
        exp_env e;

        auto start = chrono::steady_clock::now();

        for (int i = 0; i < TOTAL_SERVERS; ++i)
            conn_one_server_timed(IP_SERVER, BASE_PORT + i);

        thread read_thread, ovhd_thread;

        if (read_cmd != nullptr)
        {
            read_thread = thread([this] {
                redis_client c1(IP_SERVER, BASE_PORT);
                redis_client c2(IP_SERVER, BASE_PORT + 1);
                auto start_time = chrono::steady_clock::now();
                int i = 0;
                while (RUN_CONDITION)
                {
                    i++;
                    auto tar_time = start_time + chrono::duration<double>(i * TIME_READ);
                    this_thread::sleep_until(tar_time);
                    if (!exp_setting::compare)
                        read_cmd->exec(c1);
                    else
                    {
                        auto at1 = async([this, &c1] { return c1.exec(*read_cmd); });
                        auto at2 = async([this, &c2] { return c2.exec(*read_cmd); });
                        auto r1 = at1.get();
                        auto r2 = at2.get();
                        log.log_compare(r1, r2);
                    }
                }
            });
        }

        if (ovhd_cmd != nullptr)
        {
            ovhd_thread = thread([this] {
                redis_client cl(IP_SERVER, BASE_PORT + 1);
                auto start_time = chrono::steady_clock::now();
                int i = 0;
                while (RUN_CONDITION)
                {
                    i++;
                    auto tar_time = start_time + chrono::duration<double>(i * TIME_OVERHEAD);
                    this_thread::sleep_until(tar_time);
                    ovhd_cmd->exec(cl);
                }
            });
        }

        thread progress_thread([this] {
            constexpr int barWidth = 50;
            double progress;
            while (RUN_CONDITION)
            {
                progress = log.write_op_executed / ((double)exp_setting::total_ops);
                cout << "\r[";
                int pos = barWidth * progress;
                for (int i = 0; i < barWidth; ++i)
                {
                    if (i < pos)
                        cout << "=";
                    else if (i == pos)
                        cout << ">";
                    else
                        cout << " ";
                }
                cout << "] " << (int)(progress * 100) << "%" << flush;
                this_thread::sleep_for(chrono::seconds(1));
            }
            cout << "\r[";
            for (int i = 0; i < barWidth; ++i)
                cout << "=";
            cout << "] 100%" << endl;
        });

        for (auto &t : thds)
            if (t.joinable()) t.join();
        if (progress_thread.joinable()) progress_thread.join();

        auto end = chrono::steady_clock::now();
        auto time = chrono::duration_cast<chrono::duration<double>>(end - start).count();
        cout << time << " seconds, " << log.write_op_generated / time << " op/s\n";
        cout << log.write_op_executed << " operations actually executed on redis." << endl;

        if (read_thread.joinable()) read_thread.join();
        if (ovhd_thread.joinable()) ovhd_thread.join();

        log.write_logfiles();
    }
};

#endif  // BENCH_EXP_RUNNER_H
