//
// Created by admin on 2020/1/6.
//

#ifndef BENCH_UTIL_H
#define BENCH_UTIL_H

#include <unordered_set>
#include <mutex>
#include <condition_variable>
#include <random>
#include <sys/stat.h>
#include "constants.h"

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include "../../redis-4.0.8/deps/hiredis/hiredis.h"
#include <direct.h>

#endif

using namespace std;

int intRand(const int min, const int max)
{
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_int_distribution<int> distribution(min, max);
    return distribution(*rand_gen);
}

int intRand(const int max)
{
    return intRand(0, max - 1);
}

double doubleRand(const double min, const double max)
{
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_real_distribution<double> distribution(min, max);
    return distribution(*rand_gen);
}

double decide()
{
    return doubleRand(0.0, 1.0);
}

inline void bench_mkdir(const char *path)
{
#if defined(_WIN32)
    _mkdir(path);
#else
    mkdir(path, S_IRWXU | S_IRGRP | S_IROTH);
#endif
}

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

class record_for_collision
{
    vector<int> v[SPLIT_NUM];
    int cur = 0;
    unordered_set<int> h;
    mutex mtx;
public:

    void add(int name)
    {
        lock_guard<mutex> lk(mtx);
        if (h.find(name) == h.end())
        {
            h.insert(name);
            v[cur].push_back(name);
        }
    }

    int get()
    {
        lock_guard<mutex> lk(mtx);
        int r;
        if (h.empty())
            r = -1;
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
        for (auto n:v[cur])
            h.erase(h.find(n));
        v[cur].clear();
    }

};

class cmd
{
public:
    virtual void exec(redisContext *c) = 0;
};

class generator
{
public:
    virtual void gen_and_exec(redisContext *c) = 0;

    virtual void join() = 0;
};

class rdt_log
{
protected:
    const char *dir;
    const char *type;

    friend class generator;

public:
    rdt_log(const char *type, const char *dir) : type(type), dir(dir) {}

    virtual void write_file() = 0;
};

class __null_cmd : public cmd
{
public:
    void exec(redisContext *c) override {}
};

extern __null_cmd null_cmd;

#endif //BENCH_UTIL_H
