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
    {
        int property = 0;
        if (bold) property |= BOLD;            // NOLINT
        if (italic) property |= ITALIC;        // NOLINT
        if (underline) property |= UNDERLINE;  // NOLINT
        add_args(prev, id, content, font, size, color, property);
        list.write_op_generated++;
    }

    void handle_redis_return(const redisReply_ptr &r) override
    {
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
    {
        add_args(id, upd_type, value);
        list.write_op_generated++;
    }

    void handle_redis_return(const redisReply_ptr &r) override { list.update(id, upd_type, value); }
};

class list_remove_cmd : public list_cmd
{
private:
    string id;

public:
    list_remove_cmd(const string &type, list_log &list, string &id)
        : list_cmd(type, list, "rem"), id(id)
    {
        add_args(id);
        list.write_op_generated++;
    }

    void handle_redis_return(const redisReply_ptr &r) override { list.remove(id); }
};

class list_read_cmd : public list_cmd
{
public:
    list_read_cmd(const string &type, list_log &list) : list_cmd(type, list, "list") {}

    void handle_redis_return(const redisReply_ptr &r) override { list.read_list(r); }
};

class list_ovhd_cmd : public list_cmd
{
public:
    list_ovhd_cmd(const string &type, list_log &list) : list_cmd(type, list, "overhead") {}

    void handle_redis_return(const redisReply_ptr &r) override
    {
        list.overhead(static_cast<int>(r->integer));
    }
};

#endif  // BENCH_LIST_CMD_H
