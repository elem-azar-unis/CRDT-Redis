//
// Created by admin on 2020/7/16.
//

#ifndef BENCH_LIST_CMD_H
#define BENCH_LIST_CMD_H

#include "../util.h"
#include "list_log.h"

class list_cmd : public cmd
{
protected:
    list_log &list;

    list_cmd(const string &type, list_log &list, const char *op) : list(list)
    {
        stream << type << "l" << op << " " << type << "list";
    }
};

class list_insert_cmd : public list_cmd
{
private:
    string prev, id, content;
    int font, size, color;
    bool bold, italic, underline;

public:
    list_insert_cmd(const string &type, list_log &list, string &prev, string &id, string &content,
                    int font, int size, int color, bool bold, bool italic, bool underline)
        : list_cmd(type, list, "insert"),
          prev(prev),
          id(id),
          content(content),
          font(font),
          size(size),
          color(color),
          bold(bold),
          italic(italic),
          underline(underline)
    {}

    void exec(redis_client &c) override
    {
        int property = 0;
        if (bold) property |= BOLD;            // NOLINT
        if (italic) property |= ITALIC;        // NOLINT
        if (underline) property |= UNDERLINE;  // NOLINT
        add(prev).add(id).add(content).add(font).add(size).add(color).add(property);
        auto r = c.exec(stream.str());
        list.insert(prev, id, content, font, size, color, bold, italic, underline);
    }
};

class list_update_cmd : public list_cmd
{
private:
    string id, upd_type;
    int value;

public:
    list_update_cmd(const string &type, list_log &list, string &id, string &upd_type, int value)
        : list_cmd(type, list, "update"), id(id), upd_type(upd_type), value(value)
    {}

    void exec(redis_client &c) override
    {
        add(id).add(upd_type).add(value);
        auto r = c.exec(stream.str());
        list.update(id, upd_type, value);
    }
};

class list_remove_cmd : public list_cmd
{
private:
    string id;

public:
    list_remove_cmd(const string &type, list_log &list, string &id)
        : list_cmd(type, list, "rem"), id(id)
    {}

    void exec(redis_client &c) override
    {
        add(id);
        auto r = c.exec(stream.str());
        list.remove(id);
    }
};

class list_read_cmd : public list_cmd
{
public:
    list_read_cmd(const string &type, list_log &list) : list_cmd(type, list, "list") {}

    void exec(redis_client &c) override
    {
        auto r = c.exec(stream.str());
        list.read_list(r);
    }
};

class list_ovhd_cmd : public list_cmd
{
public:
    list_ovhd_cmd(const string &type, list_log &list) : list_cmd(type, list, "overhead") {}

    void exec(redis_client &c) override
    {
        auto r = c.exec(stream.str());
        list.overhead(static_cast<int>(r->integer));
    }
};

class list_opcount_cmd : public list_cmd
{
public:
    list_opcount_cmd(const string &type, list_log &list) : list_cmd(type, list, "opcount") {}

    void exec(redis_client &c) override
    {
        auto r = c.exec(stream.str());
        cout << r->integer << "\n";
    }
};

#endif  // BENCH_LIST_CMD_H
