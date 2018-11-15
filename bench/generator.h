//
// Created by user on 18-11-14.
//

#ifndef BENCH_GENERATOR_H
#define BENCH_GENERATOR_H

#include <random>
#include <list>
#include <unordered_set>
#include <hiredis/hiredis.h>
#include <thread>
#include <chrono>
#include <mutex>
#include "queueLog.h"
#include "constants.h"


using namespace std;

enum Type
{
    zadd = 0, zincrby = 1, zrem = 2, zmax = 3
};

class cmd
{
private:
    Type t;
    int e;
    double d;
    queueLog &ele;
    static const char *ozcmd[4] = {"ozadd", "ozincrby", "ozrem", "ozmax"};
    static const char *rzcmd[4] = {"rzadd", "rzincrby", "rzrem", "rzmax"};
    static const char **zcmd[2] = {ozcmd, rzcmd};
    enum z_type
    {
        o = 0, r = 1
    };
public:
    cmd(Type t, int e, double d, queueLog &em) : t(t), e(e), d(d), ele(em) {}

    cmd(const cmd &c) = default;

    void exec(redisContext *c)
    {
        char tmp[256];
        sprintf(tmp, "%s s %d %f", zcmd[Z_TYPE][t], e, d);
        auto r = static_cast<redisReply *>(redisCommand(c, tmp));
        switch (t)
        {
            case zadd:
                ele.add(e, d);
                break;
            case zincrby:
                ele.inc(e, d);
                break;
            case zrem:
                ele.rem(e);
                break;
            case zmax:
                int k = -1;
                double v = -1;
                if (r->elements == 2)
                {
                    k = atoi(r->element[0]->str); // NOLINT
                    v = atof(r->element[1]->str); // NOLINT
                }
                ele.max(k, v);
                break;
        }
    }
};

class generator
{
private:
    /*
    class e_inf
    {
        unordered_set<int> h;
        vector<int> a;
        mutex mtx;
    public:
        inline void add(int name)
        {
            mtx.lock();
            if (h.find(name) == h.end())
            {
                h.insert(name);
                a.push_back(name);
            }
            mtx.unlock();
        }

        inline int get()
        {
            int r;
            mtx.lock();
            if (a.empty())
                r = -1;
            else
                r = a[intRand(static_cast<const int>(a.size()))];
            mtx.unlock();
            return r;
        }

        inline void rem(int name)
        {
            mtx.lock();
            auto f = h.find(name);
            if (f != h.end())
            {
                h.erase(f);
                for (auto it = a.begin(); it != a.end(); ++it)
                    if (*it == name)
                    {
                        a.erase(it);
                        break;
                    }
            }
            mtx.unlock();
        }
    };
    */

    class c_inf
    {
        vector<int> v[SPLIT_NUM];
        int cur = 0;
        unordered_set<int> h;
        mutex mtx;
        const unsigned int max_time;

    public:
        inline int micro_seconds() { return max_time * 1000 / SPLIT_NUM; }

        inline void add(int name)
        {
            mtx.lock();
            if (h.find(name) == h.end())
            {
                h.insert(name);
                v[cur].push_back(name);
            }
            mtx.unlock();
        }

        inline int get()
        {
            int r;
            mtx.lock();
            if (h.empty())
                r = -1;
            else
            {
                int b = (cur + intRand(SPLIT_NUM)) % SPLIT_NUM;
                while (v[b].empty())
                    b = (b + 1) % SPLIT_NUM;
                r = v[b][intRand(static_cast<const int>(v[b].size()))];
            }
            mtx.unlock();
            return r;
        }

        inline void inc_rem()
        {
            mtx.lock();
            cur = (cur + 1) % SPLIT_NUM;
            for (auto n:v[cur])
                h.erase(h.find(n));
            v[cur].clear();
            mtx.unlock();
        }

        explicit c_inf(unsigned int mt) : max_time(mt) {}
    };

    c_inf add, rem;
    queueLog &ele;

    inline int get()
    {
        int r = -1;
        ele.mtx.lock();
        if (!ele.heap.empty())
            r = ele.heap[intRand(static_cast<const int>(ele.heap.size()))]->name;
        ele.mtx.unlock();
        return r;
    }

    static thread_local mt19937 rand_gen;

    static inline int intRand(const int max)
    {
        uniform_int_distribution<int> distribution(0, max - 1);
        return distribution(rand_gen);
    }


public:
    explicit generator(queueLog &e, unsigned int mt) : ele(e), add(mt), rem(mt)
    {
        thread([this] {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
            while (true)
            {
                this_thread::sleep_for(chrono::microseconds(add.micro_seconds()));
                add.inc_rem();
                rem.inc_rem();
            }
#pragma clang diagnostic pop
        });
    }
};

#endif //BENCH_GENERATOR_H
