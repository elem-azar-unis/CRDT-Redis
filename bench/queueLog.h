//
// Created by user on 18-11-13.
//

#ifndef BENCH_QUEUELOG_H
#define BENCH_QUEUELOG_H


#include <vector>
#include <unordered_map>
#include <mutex>

using namespace std;

class generator;

struct maxLog
{
    int kread, kactural;
    double vread, vactural;

    maxLog(int k, double v, int ak, double av) : kread(k), kactural(ak), vread(v), vactural(av) {}
};

class queueLog
{
private:
    class element
    {
    public:
        int name;
        double value;
        int index = -1;

        element(int k, double v) : name(k), value(v) {}

        inline bool operator<(const element &e) { return value < e.value; }

        inline bool operator<=(const element &e) { return value <= e.value; }

        inline bool operator>(const element &e) { return value > e.value; }

        inline bool operator>=(const element &e) { return value >= e.value; }

        inline bool operator==(const element &e) { return value == e.value; }

        inline bool operator!=(const element &e) { return value != e.value; }
    };

    unordered_map<int, element *> map;
    vector<element *> heap;

    mutex mtx, max_mtx;

    friend class generator;

    void shift_up(int s);

    void shift_down(int s);

public:

    vector<maxLog> max_log;

    void add(int k, double v);

    void inc(int k, double i);

    void rem(int k);

    void max(int k, double v);

};

#endif //BENCH_QUEUELOG_H
