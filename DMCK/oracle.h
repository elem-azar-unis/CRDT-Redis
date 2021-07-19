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

        explicit state_interface(redis_reply &rpl) : state_interface{rpl.get()} {}

        explicit state_interface(redisReply *rpl)
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

        virtual void print(std::ostream &out) const = 0;

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

    virtual void virtual_print(std::ostream &out) const = 0;

public:
    virtual bool check(std::vector<redis_connect> &conn, int crdt_num) = 0;

    void print(std::ostream &out = std::cout) const { virtual_print(out); }
};

class rpq_oracle : public oracle
{
private:
    struct state : public state_interface
    {
        int v_inn{0}, v_acq{0};

        explicit state(const std::string &str)
        {
            std::istringstream s{str};

            if (s.peek() == 'n')
                s.ignore(6);
            else
            {
                s >> p_ini >> v_inn >> v_acq;
                p_ini--;
                eset = true;
            }

            while (s.peek() == ' ')
                s.ignore();
            if (s.peek() == 'n')
                s.ignore();
            else
            {
                s >> t;
                tset = true;
            }
        }

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

        void print(std::ostream &out) const override
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

    private:
        bool equals(const state_interface &s) const override
        {
            // will never throw, typeid checked by base class operator==
            auto v = static_cast<const state &>(s);
            if (eset && (v_inn != v.v_inn || v_acq != v.v_acq)) return false;
            return true;
        }
    };

    state script;
    std::vector<state> server;

    void virtual_print(std::ostream &out) const override
    {
        out << "[script state] -- ";
        script.print(out);
        out << '\n';

        out << "[server state] -- ";
        for (auto &&tmp : server)
            tmp.print(out);
        out << std::endl;
    }

public:
    explicit rpq_oracle(const std::string &str) : script{str} {}

    bool check(std::vector<redis_connect> &conn, int crdt_num) override
    {
        std::ostringstream buf;
        buf << "rwfzestatus set" << crdt_num << " e";
        auto cmd = buf.str();
        for (auto &c : conn)
        {
            auto rpl = c.exec(cmd);
            server.emplace_back(rpl);
        }
        for (auto &s_state : server)
            if (script != s_state) return false;
        return true;
    }
};

class list_oracle : public oracle
{
private:
    struct state : public state_interface
    {
        static constexpr auto RESET = "\033[0m";
        static constexpr auto BRIGHT_YELLOW = "\033[93m";
        static constexpr auto GRAY = "\033[90m";

        int oid, v;
        std::string lt;

        explicit state(std::istringstream &s)
        {
            s >> oid >> p_ini >> v >> lt >> t;
            p_ini--;

            if (p_ini >= 0) eset = true;

            for (auto ch : t)
                if (ch != '0' && ch != ',')
                {
                    tset = true;
                    break;
                }

            while (s.peek() == ' ' || s.peek() == ';' || s.peek() == '\r' || s.peek() == '\n')
                s.ignore();
        }

        explicit state(redisReply *rpl) : state_interface{rpl}
        {
            oid = atoi(rpl->element[3]->str);
            v = atoi(rpl->element[4]->str);
            lt = rpl->element[5]->str;
        }

        void print(std::ostream &out) const override
        {
            if (&out == &std::cout) out << BRIGHT_YELLOW;
            out << oid << ' ';
            if (&out == &std::cout) out << RESET;

            if (&out == &std::cout && !eset) out << GRAY;
            out << p_ini << ' ' << v << ' ';
            if (lt == "null")
                out << lt << ' ';
            else
                out << '(' << lt << ") ";
            if (&out == &std::cout && !eset) out << RESET;

            if (&out == &std::cout && !tset) out << GRAY;
            out << '[' << t << "]";
            if (&out == &std::cout && !tset) out << RESET;

            out << " ; ";
        }

    private:
        bool equals(const state_interface &s) const override
        {
            // will never throw, typeid checked by base class operator==
            auto st = static_cast<const state &>(s);
            if (eset && (oid != st.oid || v != st.v || lt != st.lt)) return false;
            return true;
        }
    };

    std::vector<state> script;
    std::vector<std::vector<state>> server;

    void virtual_print(std::ostream &out) const override
    {
        out << "[script state] -- ";
        for (auto &&tmp : script)
            tmp.print(out);
        if (script.empty()) out << "empty";
        out << '\n';

        const char *indent = "";
        out << "[server state] -- ";
        for (auto &&tmplst : server)
        {
            out << indent;
            indent = "                  ";
            for (auto &&tmp : tmplst)
                tmp.print(out);
            if (tmplst.empty()) out << "empty";
            out << '\n';
        }
        if (server.empty()) out << '\n';
        out << std::flush;
    }

    std::vector<state> ignore_nexist(std::vector<state> &v)
    {
        std::vector<state> rtn;
        for (auto &tmp : v)
        {
            if (tmp.eset) rtn.emplace_back(tmp);
        }
        return rtn;
    }

    bool inner_check(bool ignore_ne)
    {
        if (ignore_ne)
        {
            auto script_ig = ignore_nexist(script);
            auto sv_list_ig = ignore_nexist(server[0]);
            return list_equal(script_ig, sv_list_ig);
        }
        else
            return list_equal(server[0], script);
    }

    bool list_equal(std::vector<state> &a, std::vector<state> &b)
    {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < b.size(); i++)
            if (a[i] != b[i]) return false;
        return true;
    }

public:
    explicit list_oracle(const std::string &str)
    {
        if (str.at(0) != 'n')
        {
            std::istringstream s{str};
            while (s)
                script.emplace_back(s);
        }
    }

    bool check(std::vector<redis_connect> &conn, int crdt_num) override
    {
        std::ostringstream buf;
        buf << "rwflestatusall list" << crdt_num;
        auto cmd = buf.str();
        for (auto &c : conn)
        {
            auto rpl = c.exec(cmd);
            server.emplace_back();
            auto &svlist = server.back();
            for (size_t i = 0; i < rpl->elements; i++)
                svlist.emplace_back(rpl->element[i]);
        }
        
        for (size_t i = 0; i < server.size() - 1; i++)
            if (!list_equal(server[i], server[i + 1])) return false;

        return inner_check(true);
    }
};

#endif  // DMCK_ORACLE_H
