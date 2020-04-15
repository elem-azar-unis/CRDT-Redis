//
// Created by user on 18-11-13.
//

#ifndef BENCH_QUEUELOG_H
#define BENCH_QUEUELOG_H


#include <vector>
#include <unordered_map>
#include <mutex>

#include "../util.h"

using namespace std;

class rpq_log : public rdt_log
{
private:

    struct m_log
    {
        int kread;
        double vread;
        int kactural;
        double vactural;

        m_log(int a, double b, int c, double d)
                : kread(a), vread(b), kactural(c), vactural(d) {}
    };

    struct ovhd_log
    {
        int num, ovhd;

        ovhd_log(int a, int b)
                : num(a), ovhd(b) {}
    };

    class element
    {
    public:
        int name;
        double value;
        int index = -1;

        element(int k, double v) : name(k), value(v) {}

        bool operator<(const element &e) const { return value < e.value; }

        bool operator<=(const element &e) const { return value <= e.value; }

        bool operator>(const element &e) const { return value > e.value; }

        bool operator>=(const element &e) const { return value >= e.value; }

        bool operator==(const element &e) const { return value == e.value; }

        bool operator!=(const element &e) const { return value != e.value; }
    };

    unordered_map<int, element *> map;
    vector<element *> heap;
    vector<m_log> max_log;
    vector<ovhd_log> overhead_log;

    mutex mtx, max_mtx, ovhd_mtx;

    void shift_up(int s);

    void shift_down(int s);

public:
    explicit rpq_log(const char *type) : rdt_log("rpq", type) {}

    ~rpq_log()
    {
        for(auto p:heap)
            delete p;
    }

    int random_get();

    void add(int k, double v);

    void inc(int k, double i);

    void rem(int k);

    void max(int k, double v);

    void overhead(int o);

    void write_file() override;

};

#endif //BENCH_QUEUELOG_H
