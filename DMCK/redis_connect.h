//
// Created by yqzhang on 2021/3/10.
//

#ifndef DMCK_REDIS_CONNECT_H
#define DMCK_REDIS_CONNECT_H

#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

#if defined(__linux__)
#include <hiredis/hiredis.h>

#elif defined(_WIN32)

#include "../redis-6.0.5/deps/hiredis/hiredis.h"

#endif

using redisReply_ptr = std::unique_ptr<redisReply, decltype(freeReplyObject) *>;

class redis_connect
{
private:
    const char *ip;
    const int port;
    int size, id;
    redisContext *client = nullptr, *server_instruct = nullptr, *server_listen = nullptr;

    void connect(redisContext *&c)
    {
        if (c != nullptr) redisFree(c);
        c = redisConnect(ip, port);
        if (c == nullptr || c->err)
        {
            if (c)
            {
                std::cout << "\nError for redisConnect: " << c->errstr << ", ip:" << ip
                          << ", port:" << port << std::endl;
                redisFree(c);
                c = nullptr;
            }
            else
                std::cout << "\nCan't allocate redis context" << std::endl;
            exit(-1);
        }
    }

    void connect_server_instruct()
    {
        std::ostringstream stream;
        connect(server_instruct);
        stream << "repltest " << size << " " << id;
        exec(stream.str(), server_instruct);
    }

    void connect_server_listen()
    {
        std::ostringstream stream;
        connect(server_listen);
        stream << "repltest";
        exec(stream.str(), server_listen);
    }

    inline void connect_client() { connect(client); }

    inline void reconnect(redisContext *&c)
    {
        if (c == client)
            connect_client();
        else if (c == server_instruct)
            connect_server_instruct();
        else if (c == server_listen)
            connect_server_listen();
        else
        {
            std::cout << "\nWhere does this redisContext come from?" << std::endl;
            exit(-1);
        }
    }

    void reply_error(const std::string &cmd, redisContext *&c)
    {
        std::cout << "\nSomething wrong for host " << c->tcp.host << ":" << c->tcp.port
                  << "to execute " << (c != client ? "server inner message " : "") << cmd << "\n";
        if (c->reader->err == REDIS_ERR_IO)
            std::cout << "IO error: " << strerror(errno) << std::endl;
        else
            std::cout << "errno: " << c->reader->err << ", err str: " << c->reader->errstr
                      << std::endl;
        exit(-1);
    }

    redisReply_ptr exec(const std::string &cmd, redisContext *&c)
    {
        bool retried = false;
        auto r = static_cast<redisReply *>(redisCommand(c, cmd.c_str()));
        while (r == nullptr)
        {
            if (!retried)
            {
                reconnect(c);
                retried = true;
                r = static_cast<redisReply *>(redisCommand(c, cmd.c_str()));
                continue;
            }
            reply_error(cmd, c);
        }
        return redisReply_ptr(r, freeReplyObject);
    }

public:
    redis_connect(const char *ip, int port, int size, int id)
        : ip(ip), port(port), size(size), id(id)
    {
        connect_server_instruct();
        connect_server_listen();
        connect_client();
    }

    void pass_inner_msg(redisReply_ptr &r)
    {
        std::ostringstream stream;
        for (int i = 0; i < r->elements; ++i)
            stream << (i != 0 ? " " : "") << r->element[i]->str;
        exec(stream.str(), server_instruct);
    }

    inline redisReply_ptr exec(const std::string &cmd) { return exec(cmd, client); }

    redisReply_ptr get_inner_msg()
    {
        void *reply;
        if (redisGetReply(server_listen, &reply) != REDIS_OK)
            reply_error("func: get_inner_msg()", server_listen);
        return redisReply_ptr(static_cast<redisReply *>(reply), freeReplyObject);
    }

    ~redis_connect()
    {
        if (client != nullptr) redisFree(client);
        if (server_instruct != nullptr) redisFree(server_instruct);
        if (server_listen != nullptr) redisFree(server_listen);
    }
};

#endif  // DMCK_REDIS_CONNECT_H
