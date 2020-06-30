//
// Created by admin on 2020/6/30.
//

#ifndef BENCH_RPQ_EXP_H
#define BENCH_RPQ_EXP_H

#include "rpq_basics.h"
#include "../exp_setting.h"
#include "../util.h"

class rpq_exp : public rdt_exp<rpq_type>
{
private:
    static exp_setting::default_setting rpq_setting;

    void exp_impl(rpq_type type, const char *pattern) override;

public:
    rpq_exp() : rdt_exp<rpq_type>(rpq_setting)
    {
        add_type(rpq_type::r);
        add_type(rpq_type::rwf);
        add_pattern("ardominant");
    }
};


#endif //BENCH_RPQ_EXP_H
