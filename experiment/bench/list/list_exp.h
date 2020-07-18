//
// Created by admin on 2020/7/17.
//

#ifndef BENCH_LIST_EXP_H
#define BENCH_LIST_EXP_H

#include "../exp_setting.h"
#include "../util.h"
#include "list_basics.h"

class list_exp : public rdt_exp<list_type>
{
private:
    static exp_setting::default_setting list_setting;

    void exp_impl(list_type type, const char *pattern) override;

public:
    list_exp() : rdt_exp<list_type>(list_setting)
    {
        add_type(list_type::r);
        add_type(list_type::rwf);
        // TODO add_pattern("???");
    }
};

#endif  // BENCH_LIST_EXP_H
