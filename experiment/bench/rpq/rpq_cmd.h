//
// Created by admin on 2020/1/8.
//

#ifndef BENCH_RPQ_CMD_H
#define BENCH_RPQ_CMD_H

#include "../util.h"
#include "rpq_basics.h"
#include "rpq_log.h"

enum class rpq_op_type
{
    add,
    incrby,
    rem,
    max,
    overhead,
    opcount
};

class rpq_cmd : public cmd
{
private:
    const string &type;
    rpq_op_type t;
    int e;
    double d;
    rpq_log &ele;

public:
    rpq_cmd(const string &type, rpq_op_type t, int e, double d, rpq_log &em)
        : type(type), t(t), e(e), d(d), ele(em)
    {}

    void exec(redis_client &c) override
    {
        ostringstream stream;
        stream << type;
        switch (t)
        {
            case rpq_op_type::add:
                stream << "zadd " << type << "rpq " << e << " " << d;
                break;
            case rpq_op_type::incrby:
                stream << "zincrby " << type << "rpq " << e << " " << d;
                break;
            case rpq_op_type::rem:
                stream << "zrem " << type << "rpq " << e;
                break;
            case rpq_op_type::max:
                stream << "zmax " << type << "rpq";
                break;
            case rpq_op_type::overhead:
                stream << "zoverhead " << type << "rpq";
                break;
            case rpq_op_type::opcount:
                stream << "zopcount";
                break;
        }
        auto r = c.exec(stream.str());
        switch (t)
        {
            case rpq_op_type::add:
                ele.add(e, d);
                break;
            case rpq_op_type::incrby:
                ele.inc(e, d);
                break;
            case rpq_op_type::rem:
                ele.rem(e);
                break;
            case rpq_op_type::max: {
                int k = -1;
                double v = -1;
                if (r->elements == 2)
                {
                    k = atoi(r->element[0]->str);  // NOLINT
                    v = atof(r->element[1]->str);  // NOLINT
                }
                ele.max(k, v);
                break;
            }
            case rpq_op_type::overhead:
                ele.overhead(static_cast<int>(r->integer));
                break;
            case rpq_op_type::opcount:
                cout << r->integer << "\n";
                break;
        }
    }
};

#endif  // BENCH_RPQ_CMD_H
