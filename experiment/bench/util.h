//
// Created by admin on 2020/1/6.
//

#ifndef BENCH_UTIL_H
#define BENCH_UTIL_H

#include <unordered_set>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <random>
#include <sys/stat.h>

#include "constants.h"
#include "exp_setting.h"

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include "../../redis-6.0.5/deps/hiredis/hiredis.h"
#include <direct.h>

#endif

using namespace std;

int intRand(int min, int max);

int intRand(int max);

double doubleRand(double min, double max);

double decide();

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

    ~redis_client()
    {
        redisFree(c);
    }
};

class cmd
{
public:
    virtual void exec(redis_client &c) = 0;
};

template<class T>
class generator
{
protected:
    class record_for_collision
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
            T r;
            if (h.empty())
                r = fail;
            else
            {
                int b = (cur + intRand(SPLIT_NUM)) % SPLIT_NUM;
                while (v[b].empty())
                    b = (b + 1) % SPLIT_NUM;
                r = v[b][intRand(static_cast<const int>(v[b].size()))];
            }
            return r;
        }

        void inc_rem()
        {
            lock_guard<mutex> lk(mtx);
            cur = (cur + 1) % SPLIT_NUM;
            for (auto &n:v[cur])
                h.erase(h.find(n));
            v[cur].clear();
        }

    };

private:
    vector<record_for_collision *> records;
    thread maintainer;
    volatile bool running = true;

protected:
    void add_record(record_for_collision &r)
    {
        records.emplace_back(&r);
    }

    void start_maintaining_records()
    {
        maintainer = thread([this] {
            while (running)
            {
                this_thread::sleep_for(chrono::microseconds(SLP_TIME_MICRO));
                for (auto &r:records)
                    r->inc_rem();
            }
        });
    }

public:
    virtual void gen_and_exec(redis_client &c) = 0;

    ~generator()
    {
        running = false;
        maintainer.join();
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

public:
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

        sprintf(dir, "%s/%s:%d,%d,(%d,%d)", dir, type, TOTAL_SERVERS,
                exp_setting::op_per_sec, exp_setting::delay, exp_setting::delay_low);
        bench_mkdir(dir);
    }

    virtual void write_file() = 0;
};

#endif //BENCH_UTIL_H
