//
// Created by admin on 2020/1/7.
//

#ifndef BENCH_EXP_RUNNER_H
#define BENCH_EXP_RUNNER_H

#include <thread>

#include "util.h"
#include "constants.h"
#include "exp_env.h"

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include "../../redis-4.0.8/deps/hiredis/hiredis.h"
#include <direct.h>

#endif

using namespace std;
//extern const char *ips[3];

template<class T>
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
            while (n == 0)
                cv.wait(lk);
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
    generator<T> &gen;
    cmd *read_cmd = nullptr;
    cmd *ovhd_cmd = nullptr;
    cmd *opcount_cmd = nullptr;

    vector<thread *> thds;
    vector<task_queue *> tasks;

    void conn_one_server_timed(const char *ip, int port)
    {
        for (int i = 0; i < THREAD_PER_SERVER; ++i)
        {
            auto task = new task_queue();
            auto t = new thread([this, ip, port, task] {
                redisContext *c = redisConnect(ip, port);
                if (c == nullptr || c->err)
                {
                    if (c)
                    {
                        printf("Error: %s, ip:%s, port:%d\n", c->errstr, ip, port);
                    }
                    else
                    {
                        printf("Can't allocate redis context\n");
                    }
                    exit(-1);
                }
                for (int times = 0; times < OP_PER_THREAD; ++times)
                {
                    task->worker();
                    gen.gen_and_exec(c);
                }
                redisFree(c);
            });
            thds.emplace_back(t);
            tasks.emplace_back(task);
        }
    }

public:
    exp_runner(rdt_log &log, generator<T> &gen) : gen(gen), log(log) {}

    void set_cmd_read(cmd &readCmd)
    {
        read_cmd = &readCmd;
    }

    void set_cmd_ovhd(cmd &ovhdCmd)
    {
        ovhd_cmd = &ovhdCmd;
    }

    void set_cmd_opcount(cmd &opcountCmd)
    {
        opcount_cmd = &opcountCmd;
    }

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
                for (auto t:tasks)
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
        thread *read_thread = nullptr, *ovhd_thread = nullptr;

        if (read_cmd != nullptr)
        {
            rb = true;
            read_thread = new thread([this, &rb] {
                redisContext *cl = redisConnect(IP_SERVER, BASE_PORT);
                while (rb)
                {
                    this_thread::sleep_for(chrono::seconds(TIME_MAX));
                    read_cmd->exec(cl);
                }
                redisFree(cl);
            });
        }

        if (ovhd_cmd != nullptr)
        {
            ob = true;
            ovhd_thread = new thread([this, &ob] {
                redisContext *cl = redisConnect(IP_SERVER, BASE_PORT + 1);
                while (ob)
                {
                    this_thread::sleep_for(chrono::seconds(TIME_OVERHEAD));
                    ovhd_cmd->exec(cl);
                }
                if (opcount_cmd != nullptr)
                    opcount_cmd->exec(cl);
                redisFree(cl);
            });
        }

        timer.join();
        for (auto t:thds)
            t->join();

        auto end = chrono::steady_clock::now();
        auto time = chrono::duration_cast<chrono::duration<double>>(end - start).count();
        printf("%f seconds, %f op/s\n", time, exp_setting::total_ops / time);

        printf("ending.\n");

        if (read_thread != nullptr)
        {
            rb = false;
            read_thread->join();
            delete read_thread;
        }
        if (ovhd_thread != nullptr)
        {
            ob = false;
            ovhd_thread->join();
            delete ovhd_thread;
        }
        gen.stop_and_join();

        log.write_file();

        for (auto t :thds)
            delete t;
        for (auto t :tasks)
            delete t;

        this_thread::sleep_for(chrono::seconds(5));
    }
};

#endif //BENCH_EXP_RUNNER_H
