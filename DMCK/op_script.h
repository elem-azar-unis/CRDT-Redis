//
// Created by yqzhang on 2021/5/29.
//

#ifndef DMCK_OP_SCRIPT_H
#define DMCK_OP_SCRIPT_H

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "redis_connect.h"

class op_script
{
protected:
    enum class phase
    {
        FULL,
        EFFECT
    };

    struct op
    {
        std::string full_op;
        redisReply_ptr effect_op{nullptr, nullptr};
        explicit op(std::string &&full_op) : full_op{std::move(full_op)} {}
    };

    struct step
    {
        phase type;
        int opid, pid;
        step(const phase &type, const int &opid, const int &pid) : type{type}, opid{opid}, pid{pid}
        {}
    };

    std::vector<op> optable;
    std::vector<step> steps;

    virtual bool construct_optable(std::istringstream &s, int crdt_num,
                                   const std::string &type) = 0;

    void full_construct(std::string &str, int crdt_num)
    {
        std::istringstream s{str};
        int opid = 0, pid = 0;
        std::string type;
        while (s)
        {
            s >> opid >> pid >> type;
            opid--;
            pid--;
            if (type == "effect")
                steps.emplace_back(phase::EFFECT, opid, pid);
            else
            {
                steps.emplace_back(phase::FULL, opid, pid);
                if (!construct_optable(s, crdt_num, type))
                {
                    std::cout << "Unknown input for op_script parsing: ";
                    std::cout << opid << " " << pid << " " << type << std::endl;
                    exit(-1);
                }
            }
            while (s.peek() == ' ' || s.peek() == ';')
                s.ignore();
        }
    }

public:
    void print() const
    {
        std::cout << "[op_table]:\n";
        for (auto &&tmp : optable)
            std::cout << "  - " << tmp.full_op << " ; " << inner_rpl_to_str(tmp.effect_op) << '\n';
        std::cout << "[steps]:\n";
        for (auto &&tmp : steps)
        {
            std::cout << "  - " << (tmp.type == phase::FULL ? "FULL" : "EFFECT");
            std::cout << ' ' << tmp.opid << ' ' << tmp.pid << '\n';
        }
        std::cout << std::flush;
    }

    void run(std::vector<redis_connect> &conn)
    {
        for (auto &&stp : steps)
        {
            if (stp.type == phase::FULL)
            {
                conn[stp.pid].exec(optable[stp.opid].full_op);
                optable[stp.opid].effect_op = conn[stp.pid].get_inner_msg();
            }
            else
                conn[stp.pid].pass_inner_msg(optable[stp.opid].effect_op);
        }
    }
};

class rpq_op_script : public op_script
{
private:
    bool construct_optable(std::istringstream &s, int crdt_num, const std::string &type) override
    {
        int param;
        std::ostringstream buf;
        if (type == "Add")
        {
            s >> param;
            buf << "rwfzadd set" << crdt_num << " e " << param;
        }
        else if (type == "Inc")
        {
            s >> param;
            buf << "rwfzincrby set" << crdt_num << " e " << param;
        }
        else if (type == "Rmv")
            buf << "rwfzrem set" << crdt_num << " e";
        else
            return false;
        optable.emplace_back(buf.str());
        return true;
    }

public:
    rpq_op_script(std::string &str, int crdt_num) { full_construct(str, crdt_num); }
};

#endif  // DMCK_OP_SCRIPT_H
