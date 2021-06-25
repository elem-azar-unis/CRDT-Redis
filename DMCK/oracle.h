//
// Created by admin on 2021/5/30.
//

#ifndef DMCK_ORACLE_H
#define DMCK_ORACLE_H

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "redis_connect.h"

class oracle
{
protected:
    struct state_interface
    {
        bool eset{false}, tset{false};
        int p_ini{-1};
        std::string t;

        state_interface() = default;

        explicit state_interface(redis_reply &rpl)
        {
            if (rpl->type != REDIS_REPLY_ARRAY)
            {
                std::cout << "--Error: Wrong reply type \'" << rpl->type
                          << "\' for parsing CRDT state." << std::endl;
                exit(-1);
            }
            if (rpl->elements != 0 && rpl->elements < 3)
            {
                std::cout << "--Error: Wrong reply length \'" << rpl->type
                          << "\' for parsing CRDT state." << std::endl;
                exit(-1);
            }

            if (rpl->elements != 0)
            {
                // eset
                auto ch = value(rpl->element[0]);
                if (*ch != '-')
                {
                    eset = true;
                    p_ini = atoi(ch);
                }

                // tset
                if (rpl->element[2]->type != REDIS_REPLY_STRING)
                {
                    std::cout << "--Error: Wrong reply type \'" << rpl->element[2]->type
                              << "\' to get CRDT rh-vec." << std::endl;
                    exit(-1);
                }
                char *ch_t = rpl->element[2]->str;
                while (*ch_t != '|')
                {
                    if (*ch_t != '0' && *ch_t != ',') tset = true;
                    ch_t++;
                }
                *ch_t = '\0';
                if (tset) t = rpl->element[2]->str;
            }
        }

        void print(std::ostream &out = std::cout) { virtual_print(out); }

        bool operator==(const state_interface &other) const
        {
            if (typeid(*this) != typeid(other)) return false;
            if (eset != other.eset || tset != other.tset) return false;
            if (eset && p_ini != other.p_ini) return false;
            if (tset && t != other.t) return false;
            return equals(other);
        }

        bool operator!=(const state_interface &other) const { return !(*this == other); }

    protected:
        virtual void virtual_print(std::ostream &out) const = 0;
        virtual bool equals(const state_interface &s) const = 0;

        static const char *value(redisReply *r)
        {
            if (r->type != REDIS_REPLY_STRING)
            {
                std::cout << "--Error: Wrong reply type \'" << r->type
                          << "\' to get CRDT state value." << std::endl;
                exit(-1);
            }
            auto ch = r->str;
            while (*ch != ':')
                ch++;
            return ch + 1;
        }
    };

    std::unique_ptr<state_interface> script;
    std::vector<std::unique_ptr<state_interface>> server;

    virtual void parse(const std::string &str) = 0;

public:
    virtual bool check(std::vector<redis_connect> &conn, int crdt_num) = 0;

    void print(std::ostream &out = std::cout) const
    {
        out << "[script state] -- ";
        script->print();
        out << '\n';

        out << "[server state] -- ";
        for (auto &&tmp : server)
            tmp->print();
        out << std::endl;
    }
};

class rpq_oracle : public oracle
{
private:
    struct state : public state_interface
    {
        int v_inn{0}, v_acq{0};

        state() = default;

        explicit state(redis_reply &rpl) : state_interface{rpl}
        {
            if (eset)
            {
                auto ch = value(rpl->element[3]);
                v_inn = static_cast<int>(atof(ch));
                ch = value(rpl->element[4]);
                v_acq = static_cast<int>(atof(ch));
            }
        }

    private:
        void virtual_print(std::ostream &out) const override
        {
            if (eset)
                out << p_ini << ' ' << v_inn << ' ' << v_acq << ' ';
            else
                out << "n n n ";
            if (tset)
                out << '[' << t << "] ; ";
            else
                out << "n ; ";
        }

        bool equals(const state_interface &s) const override
        {
            // will never throw, typeid checked by base class operator==
            auto v = static_cast<const state &>(s);
            if (eset && (v_inn != v.v_inn || v_acq != v.v_acq)) return false;
            return true;
        }
    };

    void parse(const std::string &str) override
    {
        std::istringstream s{str};
        while (s)
        {
            script = std::unique_ptr<state_interface>(new state);
            // I'm really sure not to use dynamic_cast
            auto &new_state = static_cast<state &>(*script);

            if (s.peek() == 'n')
                s.ignore(6);
            else
            {
                s >> new_state.p_ini >> new_state.v_inn >> new_state.v_acq;
                new_state.p_ini--;
                new_state.eset = true;
            }

            while (s.peek() == ' ')
                s.ignore();
            if (s.peek() == 'n')
                s.ignore();
            else
            {
                s >> new_state.t;
                new_state.tset = true;
            }

            while (s.peek() == ' ' || s.peek() == ';' || s.peek() == '\r' || s.peek() == '\n')
                s.ignore();
        }
    }

public:
    explicit rpq_oracle(const std::string &str) { parse(str); }

    bool check(std::vector<redis_connect> &conn, int crdt_num) override
    {
        std::ostringstream buf;
        buf << "rwfzestatus set" << crdt_num << " e";
        auto cmd = buf.str();
        for (auto &c : conn)
        {
            auto rpl = c.exec(cmd);
            server.emplace_back(new state{rpl});
        }
        for (int i = 0; i < server.size(); i++)
            if (*script != *server[i]) return false;
        return true;
    }
};

#endif  // DMCK_ORACLE_H
