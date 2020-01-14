//
// Created by admin on 2020/1/13.
//

#ifndef BENCH_LIST_LOG_H
#define BENCH_LIST_LOG_H

#include <string>
#include <list>
#include <unordered_map>
#include <mutex>

#include "../util.h"

using namespace std;

class list_log : public rdt_log
{
private:
    struct record
    {
        int num;
        double value;

        record(int n, double v) : num(n), value(v) {}
    };

    struct element
    {
        string name;
        string content;
        int font,size,  color;
        bool bold, italic, underline;

        element(char *name, char *content, int font, int size, int color,
                bool bold, bool italic, bool underline) :
                name(name), content(content), size(size), font(font), color(color),
                bold(bold), italic(italic), underline(underline) {}
    };

    static double diff(const element& e, const redisReply * r);
};


#endif //BENCH_LIST_LOG_H
