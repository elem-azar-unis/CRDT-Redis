//
// Created by admin on 2020/1/6.
//

#ifndef BENCH_UTIL_H
#define BENCH_UTIL_H

#include <sys/stat.h>

#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <mutex>
#include <random>
#include <sstream>
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

string strRand(int max_len = 16);

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
            if (c) { cout << "Error: " << c->errstr << ", ip:" << ip << ", port:" << port << endl; }
            else
            {
                cout << "Can't allocate redis context" << endl;
            }
            exit(-1);
        }
    }

    redisReply_ptr exec(const string &cmd)
    {
        auto r = static_cast<redisReply *>(redisCommand(c, cmd.c_str()));
        if (r == nullptr)
        {
            cout << "host " << c->tcp.host << ":" << c->tcp.port << " terminated.\n"
                 << "executing " << cmd << endl;
            exit(-1);
        }
        return redisReply_ptr(r, freeReplyObject);
    }

    ~redis_client() { redisFree(c); }
};

class cmd
{
protected:
    ostringstream stream;

    cmd() = default;

public:
    inline cmd &add(const string &s)
    {
        stream << " " << s;
        return *this;
    }

    inline cmd &add(int s)
    {
        stream << " " << s;
        return *this;
    }

    inline cmd &add(double s)
    {
        stream << " " << s;
        return *this;
    }

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

        T get(T &&fail)
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

    ~generator()
    {
        running = false;
        if (maintainer.joinable()) maintainer.join();
    }

public:
    virtual void gen_and_exec(redis_client &c) = 0;
};

class rdt_log
{
private:
    static inline void bench_mkdir(const string &path)
    {
#if defined(_WIN32)
        _mkdir(path.c_str());
#else
        mkdir(path.c_str(), S_IRWXU | S_IRGRP | S_IROTH);
#endif
    }

protected:
    string dir;

    rdt_log(const char *CRDT_name, const string &type)
    {
        ostringstream stream;
        stream << "../result/" << CRDT_name;
        bench_mkdir(stream.str());

        if (exp_setting::type == exp_setting::exp_type::pattern)
            stream << "/" << exp_setting::pattern_name;
        else
            stream << "/" << exp_setting::type_str[static_cast<int>(exp_setting::type)];
        bench_mkdir(stream.str());

        stream << "/" << exp_setting::round_num;
        bench_mkdir(stream.str());

        stream << "/" << type << "_" << TOTAL_SERVERS << "," << exp_setting::op_per_sec << ",("
               << exp_setting::delay << "," << exp_setting::delay_low << ")";
        dir = stream.str();
        bench_mkdir(dir);
    }

public:
    int write_op_executed=0;
    virtual void write_file() = 0;
};

class rdt_exp
{
private:
    const char *rdt_type;
    exp_setting::default_setting &rdt_exp_setting;

    void test_delay(int round)
    {
        for (int delay = rdt_exp_setting.delay_e.start; delay <= rdt_exp_setting.delay_e.end;
             delay += rdt_exp_setting.delay_e.step)
            for (auto &type : rdt_types)
                delay_fix(delay, round, type);
    }

    void test_replica(int round)
    {
        for (int replica = rdt_exp_setting.replica_e.start;
             replica <= rdt_exp_setting.replica_e.end; replica += rdt_exp_setting.replica_e.step)
            for (auto &type : rdt_types)
                replica_fix(replica, round, type);
    }

    void test_speed(int round)
    {
        for (int speed = rdt_exp_setting.speed_e.start; speed <= rdt_exp_setting.speed_e.end;
             speed += rdt_exp_setting.speed_e.step)
            for (auto &type : rdt_types)
                speed_fix(speed, round, type);
    }

protected:
    vector<string> rdt_types;
    vector<string> rdt_patterns;

    void add_type(const char *type) { rdt_types.emplace_back(type); }

    void add_pattern(const char *pattern) { rdt_patterns.emplace_back(pattern); }

    explicit rdt_exp(exp_setting::default_setting &rdt_st, const char *rdt_type)
        : rdt_exp_setting(rdt_st), rdt_type(rdt_type)
    {}

    virtual void exp_impl(const string &type, const string &pattern) = 0;

    inline void exp_impl(const string &type) { exp_impl(type, "default"); }

public:
    void delay_fix(int delay, int round, const string &type)
    {
        exp_setting::set_exp_subject(type.c_str(), rdt_type);
        exp_setting::set_default(&rdt_exp_setting);
        exp_setting::set_delay(round, delay, delay / 5);
        exp_impl(type);
        exp_setting::set_default(nullptr);
        exp_setting::set_exp_subject(nullptr, nullptr);
    }

    void replica_fix(int s_p_c, int round, const string &type)
    {
        exp_setting::set_exp_subject(type.c_str(), rdt_type);
        exp_setting::set_default(&rdt_exp_setting);
        exp_setting::set_replica(round, 3, s_p_c);
        exp_impl(type);
        exp_setting::set_default(nullptr);
        exp_setting::set_exp_subject(nullptr, nullptr);
    }

    void speed_fix(int speed, int round, const string &type)
    {
        exp_setting::set_exp_subject(type.c_str(), rdt_type);
        exp_setting::set_default(&rdt_exp_setting);
        exp_setting::set_speed(round, speed);
        exp_impl(type);
        exp_setting::set_default(nullptr);
        exp_setting::set_exp_subject(nullptr, nullptr);
    }

    void pattern_fix(const string &pattern, const string &type)
    {
        exp_setting::set_exp_subject(type.c_str(), rdt_type);
        exp_setting::set_default(&rdt_exp_setting);
        exp_setting::set_pattern(pattern);
        exp_impl(type, pattern);
        exp_setting::set_default(nullptr);
        exp_setting::set_exp_subject(nullptr, nullptr);
    }

    void exp_start_all(int rounds)
    {
        auto start = chrono::steady_clock::now();

        for (auto &p : rdt_patterns)
            for (auto &t : rdt_types)
                pattern_fix(p, t);

        for (int i = 0; i < rounds; i++)
        {
            test_delay(i);
            test_replica(i);
            test_speed(i);
        }

        auto end = chrono::steady_clock::now();
        auto time = chrono::duration_cast<chrono::duration<double>>(end - start).count();
        cout << "total time: " << time << " seconds" << endl;
    }
};

#endif  // BENCH_UTIL_H
