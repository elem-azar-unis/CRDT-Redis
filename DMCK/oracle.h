//
// Created by admin on 2021/5/30.
//

#ifndef DMCK_ORACLE_H
#define DMCK_ORACLE_H

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

        virtual void print(std::ostream &out = std::cout) const = 0;
    };
    std::vector<std::unique_ptr<state_interface>> script, server;

    virtual void parse(const std::string &str) = 0;

public:
    virtual bool check(std::vector<redis_connect> &conn, int crdt_num) = 0;

    void print(std::ostream &out = std::cout) const
    {
        out << "[script state] -- ";
        for (auto &&tmp : script)
            tmp->print();
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

        void print(std::ostream &out = std::cout) const override
        {
            if (eset)
                out << p_ini << ' ' << v_inn << ' ' << v_acq << ' ';
            else
                out << "n n n ";
            if (tset)
                out << t << " ; ";
            else
                out << "n ; ";
        }
    };

    void parse(const std::string &str) override
    {
        std::istringstream s{str};
        while (s)
        {
            script.emplace_back(new state);
            // I'm really sure not to use dynamic_cast
            auto &new_state = static_cast<state &>(*script.back());

            if (s.peek() == 'n')
                s.ignore(6);
            else
            {
                s >> new_state.p_ini >> new_state.v_inn >> new_state.v_acq;
                new_state.p_ini--;
                new_state.eset = true;
            }
            if (s.peek() == 'n')
                s.ignore();
            else
            {
                s >> new_state.t;
                new_state.tset = true;
            }
            while (s.peek() == ' ' || s.peek() == ';')
                s.ignore();
        }
    }

public:
    explicit rpq_oracle(const std::string &str) { parse(str); }

    bool check(std::vector<redis_connect> &conn, int crdt_num) override
    {
        // TODO
        return true;
    }
};

#endif  // DMCK_ORACLE_H
