//
// Created by admin on 2020/1/8.
//

#include "rpq_generator.h"

void rpq_generator::gen_and_exec(redisContext *c)
{
    rpq_op_type t;
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
            e = add.get(-1);
            if (e == -1)
            {
                e = gen_element();
                add.add(e);
            }
        }
        else if (conf < PAR)
        {
            e = rem.get(-1);
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
        e = ele.random_get();
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
            e = add.get(-1);
            if (e == -1)
            {
                e = ele.random_get();
                if (e == -1)return;
            }
            rem.add(e);
        }
        else if (conf < PRR)
        {
            e = rem.get(-1);
            if (e == -1)
            {
                e = ele.random_get();
                if (e == -1)return;
                rem.add(e);
            }
        }
        else
        {
            e = ele.random_get();
            if (e == -1)return;
            rem.add(e);
        }
    }
    rpq_cmd(zt, t, e, d, ele).exec(c);
}

