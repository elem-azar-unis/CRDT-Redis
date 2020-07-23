//
// Created by admin on 2020/1/8.
//

#include "rpq_generator.h"

#define PA (pattern.PR_ADD)
#define PI (pattern.PR_ADD + pattern.PR_INC)
#define PAA (pattern.PR_ADD_CA)
#define PAR (pattern.PR_ADD_CA + pattern.PR_ADD_CR)
#define PRA (pattern.PR_REM_CA)
#define PRR (pattern.PR_REM_CA + pattern.PR_REM_CR)

rpq_generator::rpq_op_gen_pattern& rpq_generator::get_pattern(const string& name)
{
    static map<string, rpq_op_gen_pattern> patterns{{"default",
                                                     {.PR_ADD = 0.41,
                                                      .PR_INC = 0.2,
                                                      .PR_REM = 0.39,

                                                      .PR_ADD_CA = 0.15,
                                                      .PR_ADD_CR = 0.05,
                                                      .PR_REM_CA = 0.1,
                                                      .PR_REM_CR = 0.1}},

                                                    {"ardominant",
                                                     {.PR_ADD = 0.11,
                                                      .PR_INC = 0.8,
                                                      .PR_REM = 0.09,

                                                      .PR_ADD_CA = 0.15,
                                                      .PR_ADD_CR = 0.05,
                                                      .PR_REM_CA = 0.1,
                                                      .PR_REM_CR = 0.1}}};
    if (patterns.find(name) == patterns.end()) return patterns["default"];
    return patterns[name];
}

void rpq_generator::gen_and_exec(redis_client& c)
{
    int e;
    double rand = decide();
    if (rand <= PA) { gen_add().exec(c); }
    else if (rand <= PI)
    {
        e = ele.random_get();
        if (e == -1) return gen_add().exec(c);
        double d = doubleRand(-MAX_INCR, MAX_INCR);
        rpq_incrby_cmd(zt, ele, e, d).exec(c);
    }
    else
    {
        double conf = decide();
        if (conf < PRA)
        {
            e = add.get(-1);
            if (e == -1)
            {
                e = ele.random_get();
                if (e == -1) return gen_add().exec(c);
            }
            rem.add(e);
        }
        else if (conf < PRR)
        {
            e = rem.get(-1);
            if (e == -1)
            {
                e = ele.random_get();
                if (e == -1) return gen_add().exec(c);
                rem.add(e);
            }
        }
        else
        {
            e = ele.random_get();
            if (e == -1) return gen_add().exec(c);
            rem.add(e);
        }
        rpq_remove_cmd(zt, ele, e).exec(c);
    }
}
rpq_add_cmd rpq_generator::gen_add()
{
    int e;
    double d = doubleRand(0, MAX_INIT);
    double conf = decide();
    if (conf < PAA)
    {
        e = add.get(-1);
        if (e == -1)
        {
            e = intRand(MAX_ELE);
            add.add(e);
        }
    }
    else if (conf < PAR)
    {
        e = rem.get(-1);
        if (e == -1) e = intRand(MAX_ELE);
        add.add(e);
    }
    else
    {
        e = intRand(MAX_ELE);
        add.add(e);
    }
    return rpq_add_cmd(zt, ele, e, d);
}
