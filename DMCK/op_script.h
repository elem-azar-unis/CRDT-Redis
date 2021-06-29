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
        redis_reply effect_op;
        explicit op(std::string &&full_op) : full_op{std::move(full_op)} {}
    };

    struct step
    {
        phase type;
        int opid, pid;
        step(const phase &type, const int &opid, const int &pid) : type{type}, opid{opid}, pid{pid}
        {}
    };

    const bool verbose;
    std::string check_op;
    std::vector<op> optable;
    std::vector<step> steps;

    explicit op_script(bool verbose) : verbose{verbose} {}

    virtual bool construct_optable(std::istringstream &s, int crdt_num, std::string_view type) = 0;

    void full_construct(const std::string &str, int crdt_num)
    {
        std::istringstream s{str};
        int opid{0}, pid{0};
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
                    std::cout << "--Error: Unknown input for op_script parsing: ";
                    std::cout << opid << " " << pid << " " << type << std::endl;
                    exit(-1);
                }
            }
            while (s.peek() == ' ' || s.peek() == ';' || s.peek() == '\r' || s.peek() == '\n')
                s.ignore();
        }
    }

public:
    void print(std::ostream &out = std::cout) const
    {
        out << "[op_table]:\n";
        for (auto &&tmp : optable)
            out << "  - " << tmp.full_op << " ; " << tmp.effect_op.inner_rpl_str() << '\n';
        out << "[steps]:\n";
        for (auto &&tmp : steps)
        {
            out << "  - " << (tmp.type == phase::FULL ? "FULL" : "EFFECT");
            out << ' ' << tmp.opid << ' ' << tmp.pid << '\n';
        }
        out << std::flush;
    }

    void run(std::vector<redis_connect> &conn)
    {
        for (auto &&stp : steps)
        {
            if (stp.type == phase::FULL)
            {
                auto r = conn[stp.pid].exec(optable[stp.opid].full_op);
                if (r.is_ok()) optable[stp.opid].effect_op = conn[stp.pid].get_inner_msg();
            }
            else
                conn[stp.pid].pass_inner_msg(optable[stp.opid].effect_op);
            if (verbose)
            {
                if (check_op.empty())
                {
                    std::cout << "--Error: Verbose script, check op not set" << std::endl;
                    exit(-1);
                }
                std::cout << "server " << stp.pid << " executed ["
                          << (stp.type == phase::FULL ? optable[stp.opid].full_op
                                                      : optable[stp.opid].effect_op.inner_rpl_str())
                          << "], state:\n";
                auto r = conn[stp.pid].exec(check_op);
                r.print();
            }
        }
    }
};

class rpq_op_script : public op_script
{
private:
    bool construct_optable(std::istringstream &s, int crdt_num, std::string_view type) override
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
    rpq_op_script(const std::string &str, int crdt_num, bool verbose) : op_script{verbose}
    {
        full_construct(str, crdt_num);
        if (verbose)
        {
            std::ostringstream buf;
            buf << "rwfzestatus set" << crdt_num << " e";
            check_op = buf.str();
        }
    }
};

#endif  // DMCK_OP_SCRIPT_H
