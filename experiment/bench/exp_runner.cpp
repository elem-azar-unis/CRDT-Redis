//
// Created by admin on 2020/1/8.
//

#include "exp_runner.h"

void exp_runner::conn_one_server_timed(const char *ip, int port)
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

void exp_runner::run()
{
    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);

    for (auto ip:ips)
        for (int i = 0; i < (TOTAL_SERVERS / 3); ++i)
            conn_one_server_timed(ip, 6379 + i);

    thread timer([this] {
        timeval td{}, tn{};
        gettimeofday(&td, nullptr);
        for (int times = 0; times < OP_PER_THREAD; ++times)
        {
            for (auto t:tasks)
            {
                t->add();
            }
            //this_thread::sleep_for(chrono::microseconds((int)SLEEP_TIME));
            gettimeofday(&tn, nullptr);
            auto slp_time =
                    (td.tv_sec - tn.tv_sec) * 1000000 + td.tv_usec - tn.tv_usec +
                    (long) ((times + 1) * INTERVAL_TIME);
            this_thread::sleep_for(chrono::microseconds(slp_time));
        }
    });

    volatile bool rb = true, ob = true;
    thread read([this, &rb] {
        redisContext *cl = redisConnect(ips[0], 6379);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfor-loop-analysis"
        while (rb)
        {
            this_thread::sleep_for(chrono::seconds(TIME_MAX));
            read_cmd.exec(cl);
        }
#pragma clang diagnostic pop
        redisFree(cl);
    });
    thread overhead([this, &ob] {
        redisContext *cl = redisConnect(ips[1], 6379);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfor-loop-analysis"
        while (ob)
        {
            this_thread::sleep_for(chrono::seconds(TIME_OVERHEAD));
            ovhd_cmd.exec(cl);
        }
#pragma clang diagnostic pop

        opcount_cmd.exec(cl);
        redisFree(cl);
    });

    timer.join();
    for (auto t:thds)
        t->join();

    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (t2.tv_usec - t1.tv_usec) / 1000000.0;
    printf("%f, %f\n", time_diff_sec, TOTAL_OPS / time_diff_sec);

    rb = false;
    ob = false;
    printf("ending.\n");

    read.join();
    overhead.join();
    gen.join();

    log.write_file();

    for (auto t :thds)
        delete t;
    for (auto t :tasks)
        delete t;

    this_thread::sleep_for(chrono::seconds(5));
}

