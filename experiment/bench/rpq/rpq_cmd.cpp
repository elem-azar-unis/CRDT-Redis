//
// Created by admin on 2020/1/8.
//

#include "rpq_cmd.h"

#define DEFINE_ACTION(_name) #_name,
const char *rpq_type_str[] = {RPQ_TYPE_CODEC(DEFINE_ACTION)};
#undef DEFINE_ACTION

void rpq_cmd::exec(redis_client &c)
{
    const char *type_str = rpq_type_str[static_cast<int>(zt)];
    ostringstream stream;
    stream << type_str;
    switch (t)
    {
        case rpq_op_type::add:
            stream << "zadd " << type_str << "rpq " << e << " " << d;
            break;
        case rpq_op_type::incrby:
            stream << "zincrby " << type_str << "rpq " << e << " " << d;
            break;
        case rpq_op_type::rem:
            stream << "zrem " << type_str << "rpq " << e;
            break;
        case rpq_op_type::max:
            stream << "zmax " << type_str << "rpq";
            break;
        case rpq_op_type::overhead:
            stream << "zoverhead " << type_str << "rpq";
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
