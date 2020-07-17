//
// Created by admin on 2020/1/13.
//

#ifndef BENCH_LIST_LOG_H
#define BENCH_LIST_LOG_H

#include <string>
#include <vector>
#include <tuple>
#include <list>
#include <unordered_map>
#include <mutex>

#include "../util.h"
#include "list_basics.h"

using namespace std;

class list_log : public rdt_log
{
private:
    struct element
    {
        string content;
        int font, size, color;
        bool bold, italic, underline;

        element(string &content, int font, int size, int color,
                bool bold, bool italic, bool underline) :
                content(content), size(size), font(font), color(color),
                bold(bold), italic(italic), underline(underline) {}
    };

    static double diff(const element &e, const redisReply *r);

    unordered_map<string, list<unique_ptr<element> >::iterator> ele_map;
    list<unique_ptr<element> > document;

    // len, distance
    vector<tuple<int, double> > distance_log;
    // num, overhead
    vector<tuple<int, int> > overhead_log;

    mutex mtx, dis_mtx, ovhd_mtx;

public:
    explicit list_log(list_type type) :
            rdt_log("list", list_type_str[static_cast<int>(type)]) {}

    string random_get();

    void insert(string &prev, string &name, string &content,
                int font, int size, int color, bool bold, bool italic, bool underline);

    void update(string &name, string &upd_type, int value);

    void remove(string &name);

    void read_list(redisReply_ptr &r);

    void overhead(int o);

    void write_file() override;
};


#endif //BENCH_LIST_LOG_H
