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
public:
    virtual void parse(const std::string &str) = 0;
    virtual bool check(std::vector<redis_connect> &conn, int crdt_num) = 0;
    virtual void print() = 0;
};

class rpq_oracle : public oracle
{
private:
    struct state
    {
        bool eset{false}, tset{false};
        int p_ini{-1}, v_inn{0}, v_acq{0};
        std::string t;

        void print()
        {
            if (eset)
                std::cout << p_ini << ' ' << v_inn << ' ' << v_acq << ' ';
            else
                std::cout << "n n n ";
            if (tset)
                std::cout << t << " ; ";
            else
                std::cout << "n ; ";
        }
    };

    std::vector<state> script, server;

public:
    void parse(const std::string &str) override
    {
        std::istringstream s{str};
        while (s)
        {
            script.emplace_back();
            auto &new_state = script.back();
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

    bool check(std::vector<redis_connect> &conn, int crdt_num) override;

    void print() override
    {
        for (auto &&tmp : script)
            tmp.print();
        std::cout << '\n';
        for (auto &&tmp : server)
            tmp.print();
        std::cout << std::endl;
    }
};

#endif  // DMCK_ORACLE_H
