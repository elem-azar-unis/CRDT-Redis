//
// Created by admin on 2020/4/15.
//
#include "exp_env.h"

int exp_setting::delay;
int exp_setting::delay_low;
int exp_setting::total_servers;
int exp_setting::total_ops;
int exp_setting::op_per_sec;

exp_setting::exp_type exp_setting::type;
const char *exp_setting::pattern_name;
int exp_setting::round_num;

const char *exp_setting::type_str[3] = {"speed", "replica", "delay"};
