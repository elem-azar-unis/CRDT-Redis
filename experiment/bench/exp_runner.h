//
// Created by admin on 2020/1/7.
//

#ifndef BENCH_EXP_RUNNER_H
#define BENCH_EXP_RUNNER_H

#include <thread>

#include "constants.h"
#include "exp_env.h"
#include "util.h"

#if defined(__linux__)

#include <hiredis/hiredis.h>

#elif defined(_WIN32)

#include <direct.h>

#include "../../redis-6.0.5/deps/hiredis/hiredis.h"

#endif

using namespace std;
// extern const char *ips[3];

class exp_runner
{
private:
    class task_queue
    {
        int n = 0;
        mutex m;
        condition_variable cv;

    public:
        void worker()
        {
            unique_lock<mutex> lk(m);
            cv.wait(lk, [this] { return n > 0; });
            n--;
        }

        void add()
        {
            {
                lock_guard<mutex> lk(m);
                n++;
            }
            cv.notify_all();
        }
    };

    rdt_log &log;
    generator &gen;
    cmd *read_cmd = nullptr;
    cmd *ovhd_cmd = nullptr;
    cmd *opcount_cmd = nullptr;

    vector<thread> thds;
    vector<unique_ptr<task_queue>> tasks;

    void conn_one_server_timed(const char *ip, int port)
    {
        for (int i = 0; i < THREAD_PER_SERVER; ++i)
        {
            tasks.emplace_back(new task_queue());
            auto task = tasks.back().get();
            thds.emplace_back([this, ip, port, task] {
                redis_client c(ip, port);
                for (int times = 0; times < OP_PER_THREAD; ++times)
                {
                    task->worker();
                    gen.gen_and_exec(c);
                }
            });
        }
    }

public:
    exp_runner(rdt_log &log, generator &gen) : gen(gen), log(log) {}

    void set_cmd_read(cmd &readCmd) { read_cmd = &readCmd; }

    void set_cmd_ovhd(cmd &ovhdCmd) { ovhd_cmd = &ovhdCmd; }

    void set_cmd_opcount(cmd &opcountCmd) { opcount_cmd = &opcountCmd; }

    void run()
    {
        exp_env e;

        auto start = chrono::steady_clock::now();

        for (int i = 0; i < TOTAL_SERVERS; ++i)
            conn_one_server_timed(IP_SERVER, BASE_PORT + i);

        thread timer([this] {
            auto start_time = chrono::steady_clock::now();
            for (int times = 0; times < OP_PER_THREAD; ++times)
            {
                for (auto &t : tasks)
                {
                    t->add();
                }
                auto tar_time = start_time + chrono::duration<double>((times + 1) * INTERVAL_TIME);
                this_thread::sleep_until(tar_time);
                /*
                gettimeofday(&tn, nullptr);
                auto slp_time =
                        (td.tv_sec - tn.tv_sec) * 1000000 + td.tv_usec - tn.tv_usec +
                        (long) ((times + 1) * INTERVAL_TIME);
                this_thread::sleep_for(chrono::microseconds(slp_time));
                 */
            }
        });

        volatile bool rb, ob;
        thread read_thread, ovhd_thread;

        if (read_cmd != nullptr)
        {
            rb = true;
            read_thread = thread([this, &rb] {
                redis_client cl(IP_SERVER, BASE_PORT);
                while (rb)
                {
                    this_thread::sleep_for(chrono::seconds(TIME_MAX));
                    read_cmd->exec(cl);
                }
            });
        }

        if (ovhd_cmd != nullptr)
        {
            ob = true;
            ovhd_thread = thread([this, &ob] {
                redis_client cl(IP_SERVER, BASE_PORT + 1);
                while (ob)
                {
                    this_thread::sleep_for(chrono::seconds(TIME_OVERHEAD));
                    ovhd_cmd->exec(cl);
                }
                if (opcount_cmd != nullptr) opcount_cmd->exec(cl);
            });
        }

        timer.join();
        for (auto &t : thds)
            t.join();

        auto end = chrono::steady_clock::now();
        auto time = chrono::duration_cast<chrono::duration<double>>(end - start).count();
        printf("%f seconds, %f op/s\n", time, exp_setting::total_ops / time);

        printf("ending.\n");

        if (read_thread.joinable())
        {
            rb = false;
            read_thread.join();
        }
        if (ovhd_thread.joinable())
        {
            ob = false;
            ovhd_thread.join();
        }

        log.write_file();
    }
};

#endif  // BENCH_EXP_RUNNER_H
