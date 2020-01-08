//
// Created by admin on 2020/1/8.
//

#include "rpq_cmd.h"

const char *rpq_cmd_prefix[2] = {"o", "r"};

void rpq_cmd::exec(redisContext *c)
{
    char name[128];
    sprintf(name, "%ss%d", rpq_cmd_prefix[zt], OP_PER_SEC);
    char tmp[256];
    switch (t)
    {
        case zadd:
            sprintf(tmp, "%szadd %s %d %f", rpq_cmd_prefix[zt], name, e, d);
            break;
        case zincrby:
            sprintf(tmp, "%szincrby %s %d %f", rpq_cmd_prefix[zt], name, e, d);
            break;
        case zrem:
            sprintf(tmp, "%szrem %s %d", rpq_cmd_prefix[zt], name, e);
            break;
        case zmax:
            sprintf(tmp, "%szmax %s", rpq_cmd_prefix[zt], name);
            break;
        case zoverhead:
            sprintf(tmp, "%szoverhead %s", rpq_cmd_prefix[zt], name);
            break;
        case zopcount:
            sprintf(tmp, "%szopcount", rpq_cmd_prefix[zt]);
            break;
    }
    auto r = static_cast<redisReply *>(redisCommand(c, tmp));
    if (r == nullptr)
    {
        printf("host %s:%d terminated.\nexecuting %s\n", c->tcp.host, c->tcp.port, tmp);
        exit(-1);
    }
    switch (t)
    {
        case zadd:
            ele.add(e, d);
            break;
        case zincrby:
            ele.inc(e, d);
            break;
        case zrem:
            ele.rem(e);
            break;
        case zmax:
        {
            int k = -1;
            double v = -1;
            if (r->elements == 2)
            {
                k = atoi(r->element[0]->str); // NOLINT
                v = atof(r->element[1]->str); // NOLINT
            }
            ele.max(k, v);
            break;
        }
        case zoverhead:
            ele.overhead(static_cast<int>(r->integer));
            break;
        case zopcount:
            printf("%lli\n", r->integer);
            break;
    }
    freeReplyObject(r);
}
