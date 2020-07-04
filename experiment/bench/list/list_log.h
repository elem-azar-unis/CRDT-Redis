//
// Created by admin on 2020/1/13.
//

#ifndef BENCH_LIST_LOG_H
#define BENCH_LIST_LOG_H

#include <string>
#include <list>
#include <vector>
#include <mutex>

#include "../util.h"

using std::string;
using std::list;

class list_log : public rdt_log
{
private:
    struct element
    {
        string name;
        string content;
        int font, size, color;
        bool bold, italic, underline;

        element *prev = nullptr, *next = nullptr;

        element(char *name, char *content, int font, int size, int color,
                bool bold, bool italic, bool underline) :
                name(name), content(content), size(size), font(font), color(color),
                bold(bold), italic(italic), underline(underline) {}
    };

    static double diff(const element &e, const redisReply *r);

    unordered_map<int, shared_ptr<element> > map;
    list<shared_ptr<element> > document;
    // len, distance
    vector<tuple<int, double> > distance_log;
    // num, overhead
    vector<tuple<int, int> > overhead_log;

    mutex mtx, dis_mtx, ovhd_mtx;

public:
    explicit list_log(const char *type) : rdt_log("list", type) {}

    void read_list(redisReply_ptr &r);

    void overhead(int o);
};


#endif //BENCH_LIST_LOG_H
