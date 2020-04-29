//
// Created by user on 18-11-13.
//

#ifndef BENCH_QUEUELOG_H
#define BENCH_QUEUELOG_H


#include <vector>
#include <tuple>
#include <unordered_map>
#include <mutex>

#include "../util.h"

using namespace std;

class rpq_log : public rdt_log
{
private:

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

    unordered_map<int, shared_ptr<element> > map;
    vector<shared_ptr<element> > heap;
    // kread, vread, kactural, vactural
    vector<tuple<int, double, int, double> > max_log;
    // num, ovhd
    vector<tuple<int, int> > overhead_log;

    mutex mtx, max_mtx, ovhd_mtx;

    void shift_up(int s);

    void shift_down(int s);

public:
    explicit rpq_log(const char *type) : rdt_log("rpq", type) {}

    int random_get();

    void add(int k, double v);

    void inc(int k, double i);

    void rem(int k);

    void max(int k, double v);

    void overhead(int o);

    void write_file() override;

};

#endif //BENCH_QUEUELOG_H
