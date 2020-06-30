//
// Created by admin on 2020/6/30.
//

#ifndef BENCH_RPQ_EXP_H
#define BENCH_RPQ_EXP_H

#include "rpq_basics.h"
#include "../exp_setting.h"

class rpq_exp
{
private:
    static exp_setting::default_setting rpq_setting;

    class setting_setter
    {
    public:
        setting_setter() { exp_setting::set_default(&rpq_setting); }

        ~setting_setter() { exp_setting::set_default(nullptr); }
    };

    static void test_dis(rpq_type zt);

    static void test_delay(int round);

    static void test_replica(int round);

    static void test_speed(int round);

public:

    static void delay_fix(int delay, int round, rpq_type type);

    static void replica_fix(int s_p_c, int round, rpq_type type);

    static void speed_fix(int speed, int round, rpq_type type);

    static void exp_start_all(int rounds);

};


#endif //BENCH_RPQ_EXP_H
