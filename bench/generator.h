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
#include "queue_log.h"
#include "constants.h"


using namespace std;


enum Type
{
    zadd = 0, zincrby = 1, zrem = 2, zmax = 3, zoverhead = 4
};

static const char zcmd[2] = {'o', 'r'};

enum z_type
{
    o = 0, r = 1
};

class cmd
{
private:
    Type t;
    int e;
    double d;
    queue_log &ele;

public:

    cmd(Type t, int e, double d, queue_log &em) : t(t), e(e), d(d), ele(em) {}

    cmd(const cmd &c) = default;

    void exec(redisContext *c)
    {
        char tmp[256];
        switch (t)
        {
            case zadd:
                sprintf(tmp, "%czadd s %d %f", zcmd[Z_TYPE], e, d);
                break;
            case zincrby:
                sprintf(tmp, "%czincrby s %d %f", zcmd[Z_TYPE], e, d);
                break;
            case zrem:
                sprintf(tmp, "%czrem s %d", zcmd[Z_TYPE], e);
                break;
            case zmax:
                sprintf(tmp, "%czmax s", zcmd[Z_TYPE]);
                break;
            case zoverhead:
                sprintf(tmp, "%czoverhead s", zcmd[Z_TYPE]);
                break;
        }
        auto r = static_cast<redisReply *>(redisCommand(c, tmp));
        if(r== nullptr)
        {
            printf("host %s:%d terminated.\nexecuting %s\n",c->tcp.host,c->tcp.port,tmp);
            exit(-1);
        }
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
            {
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
            case zoverhead:
                ele.overhead(static_cast<int>(r->integer));
                break;
        }
        freeReplyObject(r);
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
        void add(int name)
        {
            mtx.lock();
            if (h.find(name) == h.end())
            {
                h.insert(name);
                a.push_back(name);
            }
            mtx.unlock();
        }

        int get()
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

        void rem(int name)
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

    c_inf add, rem;
    thread maintainer;
    bool flag=true;
    queue_log &ele;

    static int intRand(const int max)
    {
        static thread_local mt19937 *rand_gen = nullptr;
        if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
        uniform_int_distribution<int> distribution(0, max - 1);
        return distribution(*rand_gen);
    }

    static double decide()
    {
        static thread_local mt19937 *rand_gen = nullptr;
        if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
        uniform_real_distribution<double> distribution(0.0, 1.0);
        return distribution(*rand_gen);
    }

    int get()
    {
        lock_guard<mutex> lk(ele.mtx);
        int r = -1;
        if (!ele.heap.empty())
            r = ele.heap[intRand(static_cast<const int>(ele.heap.size()))]->name;
        return r;
    }

    int gen_element()
    {
        return intRand(MAX_ELE);
    }

    double gen_initial()
    {
        return intRand(MAX_INIT);
    }

    double gen_increament()
    {
        static thread_local mt19937 *rand_gen = nullptr;
        if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
        uniform_int_distribution<int> distribution(-MAX_INCR, MAX_INCR);
        return distribution(*rand_gen);
    }

public:
    explicit generator(queue_log &e) : ele(e)
    {
        maintainer = thread([this] {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
            while (flag)
            {
                this_thread::sleep_for(chrono::microseconds(SLP_TIME_MICRO));
                add.inc_rem();
                rem.inc_rem();
            }
#pragma clang diagnostic pop
        });
    }

    void gen_exec(redisContext *c)
    {
        Type t;
        int e;
        double d;
        double rand = decide();
        if (rand <= PA)
        {
            t = zadd;
            d = gen_initial();
            double conf = decide();
            if (conf < PAA)
            {
                e = add.get();
                if (e == -1)
                {
                    e = gen_element();
                    add.add(e);
                }
            }
            else if (conf < PAR)
            {
                e = rem.get();
                if (e == -1)
                    e = gen_element();
                add.add(e);
            }
            else
            {
                e = gen_element();
                add.add(e);
            }
        }
        else if (rand <= PI)
        {
            t = zincrby;
            e = get();
            if (e == -1)return;
            d = gen_increament();
        }
        else
        {
            t = zrem;
            d = -1;
            double conf = decide();
            if (conf < PRA)
            {
                e = add.get();
                if (e == -1)
                {
                    e = get();
                    if (e == -1)return;
                }
                rem.add(e);
            }
            else if (conf < PRR)
            {
                e = rem.get();
                if (e == -1)
                {
                    e = get();
                    if (e == -1)return;
                    rem.add(e);
                }
            }
            else
            {
                e = get();
                if (e == -1)return;
                rem.add(e);
            }
        }
        cmd(t, e, d, ele).exec(c);
    }

    void join()
    {
        flag=false;
        maintainer.join();
    }

};


#endif //BENCH_GENERATOR_H
