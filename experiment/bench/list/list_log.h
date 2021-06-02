//
// Created by admin on 2020/1/13.
//

#ifndef BENCH_LIST_LOG_H
#define BENCH_LIST_LOG_H

#include <mutex>
#include <set>
#include <unordered_map>

#include "../util.h"

using namespace std;

constexpr auto BOLD = 1u << 0u;
constexpr auto ITALIC = 1u << 1u;
constexpr auto UNDERLINE = 1u << 2u;

class list_log : public rdt_log
{
private:
    struct element
    {
        string content;
        int font, size, color;
        bool bold, italic, underline;

        element(string &content, int font, int size, int color, bool bold, bool italic,
                bool underline)
            : content(content),
              size(size),
              font(font),
              color(color),
              bold(bold),
              italic(italic),
              underline(underline)
        {}
    };

    static double diff(const redisReply *e, const redisReply *r);

    static double diff(const element &e, const redisReply *r);

    set<string> ele_removed;
    unordered_map<string, list<unique_ptr<element>>::iterator> ele_map;
    list<unique_ptr<element>> document;

    // len, distance
    list<tuple<int, double>> distance_log;
    // num, overhead
    list<tuple<int, int>> overhead_log;

    mutex mtx, dis_mtx, ovhd_mtx;

public:
    explicit list_log(const string &type) : rdt_log("list", type) {}

    string random_get();

    string random_get_removed();

    void insert(string &prev, string &name, string &content, int font, int size, int color,
                bool bold, bool italic, bool underline);

    void update(string &name, string &upd_type, int value);

    void remove(string &name);

    void read_list(const redisReply_ptr &r);

    void overhead(int o);

    void write_logfiles() override;

    void log_compare(redisReply_ptr &r1, redisReply_ptr &r2) override;
};

#endif  // BENCH_LIST_LOG_H
