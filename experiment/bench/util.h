//
// Created by admin on 2020/1/6.
//

#ifndef BENCH_UTIL_H
#define BENCH_UTIL_H

#include <sys/stat.h>

#include <condition_variable>
#include <cstring>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_set>

#include "constants.h"
#include "exp_setting.h"

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include <direct.h>

#include "../../redis-6.0.5/deps/hiredis/hiredis.h"

#endif

using namespace std;

int intRand(int min, int max);

static inline int intRand(int max) { return intRand(0, max - 1); }

static inline bool boolRand() { return intRand(0, 1); }

string strRand();

double doubleRand(double min, double max);

static inline double decide() { return doubleRand(0.0, 1.0); }

using redisReply_ptr = unique_ptr<redisReply, decltype(freeReplyObject) *>;

class redis_client
{
private:
    redisContext *c;

public:
    redis_client(const char *ip, int port)
    {
        c = redisConnect(ip, port);
        if (c == nullptr || c->err)
        {
            if (c) { printf("Error: %s, ip:%s, port:%d\n", c->errstr, ip, port); }
            else
            {
                printf("Can't allocate redis context\n");
            }
            exit(-1);
        }
    }

    redisReply_ptr exec(const char *cmd)
    {
        auto r = static_cast<redisReply *>(redisCommand(c, cmd));
        if (r == nullptr)
        {
            printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, cmd);
            exit(-1);
        }
        return redisReply_ptr(r, freeReplyObject);
    }

    ~redis_client() { redisFree(c); }
};

class cmd
{
public:
    virtual void exec(redis_client &c) = 0;
};

class generator
{
private:
    class c_record
    {
    public:
        virtual void inc_rem() = 0;
    };

protected:
    template <class T>
    class record_for_collision : public c_record
    {
    private:
        vector<T> v[SPLIT_NUM];
        int cur = 0;
        unordered_set<T> h;
        mutex mtx;

    public:
        void add(T &name)
        {
            lock_guard<mutex> lk(mtx);
            if (h.find(name) == h.end())
            {
                h.emplace(name);
                v[cur].emplace_back(name);
            }
        }

        T get(T fail)
        {
            lock_guard<mutex> lk(mtx);
            if (h.empty()) return fail;
            int b = (cur + intRand(SPLIT_NUM)) % SPLIT_NUM;
            while (v[b].empty())
                b = (b + 1) % SPLIT_NUM;
            return v[b][intRand(static_cast<const int>(v[b].size()))];
        }

        void inc_rem() override
        {
            lock_guard<mutex> lk(mtx);
            cur = (cur + 1) % SPLIT_NUM;
            for (auto &n : v[cur])
                h.erase(n);
            v[cur].clear();
        }
    };

private:
    vector<c_record *> records;
    thread maintainer;
    volatile bool running = true;

protected:
    void add_record(c_record &r) { records.emplace_back(&r); }

    void start_maintaining_records()
    {
        maintainer = thread([this] {
            while (running)
            {
                this_thread::sleep_for(chrono::microseconds(SLP_TIME_MICRO));
                for (auto &r : records)
                    r->inc_rem();
            }
        });
    }

public:
    virtual void gen_and_exec(redis_client &c) = 0;

    ~generator()
    {
        running = false;
        if (maintainer.joinable()) maintainer.join();
    }
};

class rdt_log
{
private:
    static inline void bench_mkdir(const char *path)
    {
#if defined(_WIN32)
        _mkdir(path);
#else
        mkdir(path, S_IRWXU | S_IRGRP | S_IROTH);
#endif
    }

protected:
    char dir[64]{};

    rdt_log(const char *CRDT_name, const char *type)
    {
        sprintf(dir, "../result/%s", CRDT_name);
        bench_mkdir(dir);

        if (exp_setting::type == exp_setting::exp_type::pattern)
            sprintf(dir, "%s/%s", dir, exp_setting::pattern_name);
        else
            sprintf(dir, "%s/%s", dir, exp_setting::type_str[static_cast<int>(exp_setting::type)]);
        bench_mkdir(dir);

        sprintf(dir, "%s/%d", dir, exp_setting::round_num);
        bench_mkdir(dir);

        sprintf(dir, "%s/%s_%d,%d,(%d,%d)", dir, type, TOTAL_SERVERS, exp_setting::op_per_sec,
                exp_setting::delay, exp_setting::delay_low);
        bench_mkdir(dir);
    }

public:
    virtual void write_file() = 0;
};

template <class T>
class rdt_exp
{
private:
    exp_setting::default_setting &rdt_exp_setting;

    void test_delay(int round)
    {
        for (int delay = rdt_exp_setting.delay_e.start; delay <= rdt_exp_setting.delay_e.end;
             delay += rdt_exp_setting.delay_e.step)
            for (auto type : rdt_types)
                delay_fix(delay, round, type);
    }

    void test_replica(int round)
    {
        for (int replica = rdt_exp_setting.replica_e.start;
             replica <= rdt_exp_setting.replica_e.end; replica += rdt_exp_setting.replica_e.step)
            for (auto type : rdt_types)
                replica_fix(replica, round, type);
    }

    void test_speed(int round)
    {
        for (int speed = rdt_exp_setting.speed_e.start; speed <= rdt_exp_setting.speed_e.end;
             speed += rdt_exp_setting.speed_e.step)
            for (auto type : rdt_types)
                speed_fix(speed, round, type);
    }

protected:
    vector<T> rdt_types;
    vector<string> rdt_patterns;

    void add_type(T type) { rdt_types.emplace_back(type); }

    void add_pattern(const char *pattern) { rdt_patterns.emplace_back(pattern); }

    explicit rdt_exp(exp_setting::default_setting &rdt_st) : rdt_exp_setting(rdt_st) {}

    virtual void exp_impl(T type, const char *pattern) = 0;

public:
    void delay_fix(int delay, int round, T type)
    {
        exp_setting::set_default(&rdt_exp_setting);
        exp_setting::set_delay(round, delay, delay / 5);
        exp_impl(type, nullptr);
        exp_setting::set_default(nullptr);
    }

    void replica_fix(int s_p_c, int round, T type)
    {
        exp_setting::set_default(&rdt_exp_setting);
        exp_setting::set_replica(round, 3, s_p_c);
        exp_impl(type, nullptr);
        exp_setting::set_default(nullptr);
    }

    void speed_fix(int speed, int round, T type)
    {
        exp_setting::set_default(&rdt_exp_setting);
        exp_setting::set_speed(round, speed);
        exp_impl(type, nullptr);
        exp_setting::set_default(nullptr);
    }

    void pattern_fix(const char *pattern, T type)
    {
        exp_setting::set_default(&rdt_exp_setting);
        exp_setting::set_pattern(pattern);
        exp_impl(type, pattern);
        exp_setting::set_default(nullptr);
    }

    void exp_start_all(int rounds)
    {
        auto start = chrono::steady_clock::now();

        for (auto &p : rdt_patterns)
            for (auto t : rdt_types)
                pattern_fix(p.c_str(), t);

        for (int i = 0; i < rounds; i++)
        {
            test_delay(i);
            test_replica(i);
            test_speed(i);
        }

        auto end = chrono::steady_clock::now();
        auto time = chrono::duration_cast<chrono::duration<double>>(end - start).count();
        printf("total time: %f seconds\n", time);
    }
};

#endif  // BENCH_UTIL_H
