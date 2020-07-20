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
    {}

    void exec(redis_client &c) override
    {
        add(element).add(value);
        auto r = c.exec(stream.str());
        pq.add(element, value);
    }
};

class rpq_incrby_cmd : public rpq_cmd
{
private:
    int element;
    double value;

public:
    rpq_incrby_cmd(const string &type, rpq_log &pq, int element, double value)
        : rpq_cmd(type, pq, "incrby"), element(element), value(value)
    {}

    void exec(redis_client &c) override
    {
        add(element).add(value);
        auto r = c.exec(stream.str());
        pq.inc(element, value);
    }
};

class rpq_remove_cmd : public rpq_cmd
{
private:
    int element;

public:
    rpq_remove_cmd(const string &type, rpq_log &pq, int element)
        : rpq_cmd(type, pq, "rem"), element(element)
    {}

    void exec(redis_client &c) override
    {
        add(element);
        auto r = c.exec(stream.str());
        pq.rem(element);
    }
};

class rpq_max_cmd : public rpq_cmd
{
public:
    rpq_max_cmd(const string &type, rpq_log &pq) : rpq_cmd(type, pq, "max") {}

    void exec(redis_client &c) override
    {
        auto r = c.exec(stream.str());
        int k = -1;
        double v = -1;
        if (r->elements == 2)
        {
            k = atoi(r->element[0]->str);  // NOLINT
            v = atof(r->element[1]->str);  // NOLINT
        }
        pq.max(k, v);
    }
};

class rpq_overhead_cmd : public rpq_cmd
{
public:
    rpq_overhead_cmd(const string &type, rpq_log &pq) : rpq_cmd(type, pq, "overhead") {}

    void exec(redis_client &c) override
    {
        auto r = c.exec(stream.str());
        pq.overhead(static_cast<int>(r->integer));
    }
};

class rpq_opcount_cmd : public rpq_cmd
{
public:
    rpq_opcount_cmd(const string &type, rpq_log &pq) : rpq_cmd(type, pq, "opcount") {}

    void exec(redis_client &c) override
    {
        auto r = c.exec(stream.str());
        cout << r->integer << "\n";
    }
};

#endif  // BENCH_RPQ_CMD_H
