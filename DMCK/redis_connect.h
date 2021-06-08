//
// Created by yqzhang on 2021/3/10.
//

#ifndef DMCK_REDIS_CONNECT_H
#define DMCK_REDIS_CONNECT_H

#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#if defined(__linux__)
#include <hiredis/hiredis.h>

#elif defined(_WIN32)

#include "../redis-6.0.5/deps/hiredis/hiredis.h"

#endif

constexpr auto REDIS_SERVER = "../redis-6.0.5/src/redis-server";
constexpr auto REDIS_CONF = "../redis-6.0.5/redis.conf";
constexpr auto REDIS_CLIENT = "../redis-6.0.5/src/redis-cli";

static inline void wait_system(int milisec = 500)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(milisec));
}

class redis_reply
{
private:
    redisReply *r{nullptr};

    void print(redisReply *rpl, int depth) const
    {
        for (int i = 0; i < depth - 1; ++i)
            std::cout << "  ";
        switch (rpl->type)
        {
            case REDIS_REPLY_INTEGER:
                std::cout << rpl->integer;
                break;
            case REDIS_REPLY_DOUBLE:
                std::cout << rpl->dval;
                if (rpl->str != nullptr) std::cout << rpl->str;
                break;
            case REDIS_REPLY_STATUS:
                std::cout << "Status: " << rpl->str;
                break;
            case REDIS_REPLY_ERROR:
                std::cout << "Error: " << rpl->str;
                break;
            case REDIS_REPLY_STRING:
                std::cout << rpl->str;
                break;
            case REDIS_REPLY_VERB:
                std::cout << "Verb_" << rpl->vtype << ": " << rpl->str;
                break;
            case REDIS_REPLY_ARRAY:
                for (int i = 0; i < rpl->elements; ++i)
                    print(rpl->element[i], depth + 1);
                break;
            case REDIS_REPLY_NIL:
                std::cout << "NULL";
                break;
        }
        if (rpl->type != REDIS_REPLY_ARRAY) std::cout << std::endl;
    }

public:
    redis_reply() = default;
    redis_reply(const redis_reply &) = delete;
    redis_reply &operator=(const redis_reply &) = delete;

    explicit redis_reply(redisReply *rpl) : r{rpl} {}

    redis_reply(redis_reply &&rpl) noexcept : r{rpl.r} { rpl.r = nullptr; }

    redis_reply &operator=(redis_reply &&rpl) noexcept
    {
        if (this != &rpl)
        {
            if (r != nullptr) freeReplyObject(r);
            r = rpl.r;
            rpl.r = nullptr;
        }
        return *this;
    }

    ~redis_reply() noexcept
    {
        if (r != nullptr) freeReplyObject(r);
    }

    redisReply *get() { return r; }

    // Overload * operator
    redisReply &operator*() { return *r; }

    // Overload -> operator
    redisReply *operator->() { return r; }

    std::string inner_rpl_str() const
    {
        if (r == nullptr) return "";
        std::ostringstream stream;
        for (int i = 0; i < r->elements; ++i)
            stream << (i != 0 ? " " : "") << r->element[i]->str;
        return stream.str();
    }

    bool is_ok() const
    {
        static const std::string ok{"OK"};
        return r != nullptr && r->type == REDIS_REPLY_STATUS && ok == r->str;
    }

    void print() const
    {
        print(r, 0);
        std::cout << "----" << std::endl;
    }
};

class redis_connect
{
private:
    const char *ip;
    const int port;
    int size, id;
    redisContext *client{nullptr}, *server_instruct{nullptr}, *server_listen{nullptr};
    bool server_running{false};

    void connect(redisContext *&c)
    {
        if (c != nullptr) redisFree(c);
        c = redisConnect(ip, port);
        if (c == nullptr || c->err)
        {
            if (c)
            {
                std::cout << "\n--Error: When redisConnect: " << c->errstr << ", ip:" << ip
                          << ", port:" << port << std::endl;
                redisFree(c);
                c = nullptr;
            }
            else
                std::cout << "\n--Error: Can't allocate redis context" << std::endl;
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
            std::cout << "\n--Error: Where does this redisContext come from?" << std::endl;
            exit(-1);
        }
    }

    void reply_error(const std::string &cmd, redisContext *&c)
    {
        std::cout << "\n--Error: Something wrong for host " << c->tcp.host << ":" << c->tcp.port
                  << "to execute " << (c != client ? "server inner message " : "") << cmd << "\n";
        if (c->reader->err == REDIS_ERR_IO)
            std::cout << "  IO error: " << strerror(errno) << std::endl;
        else
            std::cout << "  errno: " << c->reader->err << ", err str: " << c->reader->errstr
                      << std::endl;
        exit(-1);
    }

    redis_reply exec(const std::string &cmd, redisContext *&c)
    {
        bool retried{false};
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
        return redis_reply{r};
    }

public:
    redis_connect(const char *ip, int port, int size, int id)
        : ip{ip}, port{port}, size{size}, id{id}
    {
        std::ostringstream stream;
        stream << REDIS_SERVER << " " << REDIS_CONF << " "
               << "--protected-mode no "
               << "--daemonize yes "
               << "--loglevel debug "
               << "--io-threads 2 "
               << "--port " << port << " "
               << "--logfile " << port << ".log "
               << "--dbfilename " << port << ".rdb "
               << "--pidfile /var/run/redis_" << port << ".pid "
               << "1>/dev/null";
        system(stream.str().c_str());
        wait_system();
        server_running = true;

        connect_server_instruct();
        connect_server_listen();
        connect_client();
    }

    redis_connect(const redis_connect &) = delete;
    redis_connect &operator=(const redis_connect &) = delete;
    redis_connect &operator=(redis_connect &&) = delete;

    redis_connect(redis_connect &&c) noexcept
        : ip{c.ip},
          port{c.port},
          size{c.size},
          id{c.id},
          client{c.client},
          server_instruct{c.server_instruct},
          server_listen{c.server_listen},
          server_running(c.server_running)
    {
        c.client = nullptr;
        c.server_instruct = nullptr;
        c.server_listen = nullptr;
        c.server_running = false;
    }

    void pass_inner_msg(redis_reply &r) { exec(r.inner_rpl_str(), server_instruct); }

    inline redis_reply exec(const std::string &cmd) { return exec(cmd, client); }

    redis_reply get_inner_msg()
    {
        void *reply;
        if (redisGetReply(server_listen, &reply) != REDIS_OK)
            reply_error("func: get_inner_msg()", server_listen);
        return redis_reply{static_cast<redisReply *>(reply)};
    }

    ~redis_connect() noexcept
    {
        if (server_instruct != nullptr) redisFree(server_instruct);
        if (server_listen != nullptr) redisFree(server_listen);
        if (client != nullptr) redisFree(client);

        if (server_running)
        {
            std::ostringstream stream;
            stream << REDIS_CLIENT << " -h 127.0.0.1 -p " << port << " SHUTDOWN NOSAVE";
            system(stream.str().c_str());
        }
    }
};

#endif  // DMCK_REDIS_CONNECT_H
