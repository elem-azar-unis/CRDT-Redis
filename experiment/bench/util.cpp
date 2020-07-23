//
// Created by admin on 2020/1/8.
//

#include "util.h"

#include "exp_env.h"
#include "exp_setting.h"

string exp_env::sudo_pwd;

const char *exp_setting::alg_type;
const char *exp_setting::rdt_type;
exp_setting::default_setting *exp_setting::default_p = nullptr;

int exp_setting::delay;
int exp_setting::delay_low;
int exp_setting::total_clusters;
int exp_setting::server_per_cluster;
int exp_setting::total_ops;
int exp_setting::op_per_sec;

exp_setting::exp_type exp_setting::type;
string exp_setting::pattern_name;
int exp_setting::round_num;

#define DEFINE_ACTION(_name) #_name,
const char *exp_setting::type_str[] = {EXP_TYPE_CODEC(DEFINE_ACTION)};
#undef DEFINE_ACTION

int intRand(int min, int max)
{
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_int_distribution<int> distribution(min, max);
    return distribution(*rand_gen);
}

string strRand(int max_len)
{
    static const char charset[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    int len = intRand(1, max_len - 1);
    ostringstream stream;
    for (int i = 0; i < len; ++i)
        stream << charset[intRand(sizeof(charset))];
    return stream.str();
}

double doubleRand(double min, double max)
{
    static thread_local mt19937 *rand_gen = nullptr;
    if (!rand_gen) rand_gen = new mt19937(clock() + hash<thread::id>()(this_thread::get_id()));
    uniform_real_distribution<double> distribution(min, max);
    return distribution(*rand_gen);
}
