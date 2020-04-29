//
// Created by admin on 2020/1/8.
//

#include "exp_env.h"
#include "exp_setting.h"
#include "util.h"

char exp_env::sudo_pwd[32];

double exp_setting::delay = 50;
double exp_setting::delay_low = 10;
int exp_setting::total_clusters = 3;
int exp_setting::server_per_cluster = 3;
int exp_setting::total_ops = 20000000;
int exp_setting::op_per_sec = 10000;

exp_setting::exp_type exp_setting::type;
const char *exp_setting::pattern_name;
int exp_setting::round_num;

const char *exp_setting::type_str[3] = {"speed", "replica", "delay"};

int intRand(int min, int max)
{
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_int_distribution<int> distribution(min, max);
    return distribution(*rand_gen);
}

int intRand(int max)
{
    return intRand(0, max - 1);
}

double doubleRand(double min, double max)
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
