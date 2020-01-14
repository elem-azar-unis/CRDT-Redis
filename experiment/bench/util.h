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

#if defined(__linux__)
#include <hiredis/hiredis.h>
#elif defined(_WIN32)

#include "../../redis-4.0.8/deps/hiredis/hiredis.h"
#include <direct.h>

#endif

using namespace std;

int intRand(int min, int max);

int intRand(int max);

double doubleRand(double min, double max);

double decide();

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

class cmd
{
public:
    virtual bool is_null() { return false; }

    virtual void exec(redisContext *c) = 0;
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
        void add(T name)
        {
            lock_guard<mutex> lk(mtx);
            if (h.find(name) == h.end())
            {
                h.insert(name);
                v[cur].push_back(name);
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
            for (auto n:v[cur])
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
        records.push_back(&r);
    }

    void start_maintaining_records()
    {
        maintainer = thread([this] {
            while (running)
            {
                this_thread::sleep_for(chrono::microseconds(SLP_TIME_MICRO));
                for (auto r:records)
                    r->inc_rem();
            }
        });
    }

public:
    virtual void gen_and_exec(redisContext *c) = 0;

    void stop_and_join()
    {
        running = false;
        maintainer.join();
    }
};

class rdt_log
{
protected:
    const char *dir;
    const char *type;

public:
    rdt_log(const char *type, const char *dir) : type(type), dir(dir) {}

    virtual void write_file() = 0;
};

class null_cmd : public cmd
{
private:
    null_cmd() = default;

    null_cmd(null_cmd const &c) {}

    null_cmd &operator=(null_cmd const &c)
    {
        if (this == &c)
            ;
    }

public:
    static null_cmd &Instance()
    {
        static null_cmd _null_cmd;
        return _null_cmd;
    }

    bool is_null() override { return true; }

    void exec(redisContext *c) override {}
};


#endif //BENCH_UTIL_H
