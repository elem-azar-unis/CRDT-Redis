//
// Created by admin on 2020/1/8.
//

#ifndef BENCH_RPQ_CMD_H
#define BENCH_RPQ_CMD_H

#include "../util.h"
#include "rpq_log.h"

class rpq_cmd : public cmd
{
protected:
    rpq_log &pq;

    rpq_cmd(const string &type, rpq_log &pq, const char *op) : pq(pq)
    {
        stream << type << "z" << op << " " << type << "rpq";
    }
};

class rpq_add_cmd : public rpq_cmd
{
private:
    int element;
    double value;

public:
    rpq_add_cmd(const string &type, rpq_log &pq, int element, double value)
        : rpq_cmd(type, pq, "add"), element(element), value(value)
    {
        add_args(element, value);
        pq.write_op_generated++;
    }

    void handle_redis_return(const redisReply_ptr &r) override { pq.add(element, value); }
};

class rpq_incrby_cmd : public rpq_cmd
{
private:
    int element;
    double value;

public:
    rpq_incrby_cmd(const string &type, rpq_log &pq, int element, double value)
        : rpq_cmd(type, pq, "incrby"), element(element), value(value)
    {
        add_args(element, value);
        pq.write_op_generated++;
    }

    void handle_redis_return(const redisReply_ptr &r) override { pq.inc(element, value); }
};

class rpq_remove_cmd : public rpq_cmd
{
private:
    int element;

public:
    rpq_remove_cmd(const string &type, rpq_log &pq, int element)
        : rpq_cmd(type, pq, "rem"), element(element)
    {
        add_args(element);
        pq.write_op_generated++;
    }

    void handle_redis_return(const redisReply_ptr &r) override { pq.rem(element); }
};

class rpq_max_cmd : public rpq_cmd
{
public:
    rpq_max_cmd(const string &type, rpq_log &pq) : rpq_cmd(type, pq, "max") {}

    void handle_redis_return(const redisReply_ptr &r) override
    {
        if (!exp_setting::compare)
        {
            int k = -1;
            double v = -1;
            if (r->elements == 2)
            {
                k = atoi(r->element[0]->str);  // NOLINT
                v = atof(r->element[1]->str);  // NOLINT
            }
            pq.max(k, v);
        }
    }
};

class rpq_overhead_cmd : public rpq_cmd
{
public:
    rpq_overhead_cmd(const string &type, rpq_log &pq) : rpq_cmd(type, pq, "overhead") {}

    void handle_redis_return(const redisReply_ptr &r) override
    {
        pq.overhead(static_cast<int>(r->integer));
    }
};

#endif  // BENCH_RPQ_CMD_H
