//
// Created by yqzhang on 2021/5/29.
//

#ifndef DMCK_OP_SCRIPT_HPP
#define DMCK_OP_SCRIPT_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
        std::string full_op, effect_op;
        op(const std::string &full_op) : full_op(full_op) {}
    };

    struct step
    {
        phase type;
        int opid, pid;
        step(const phase &type, const int &opid, const int &pid) : type(type), opid(opid), pid(pid)
        {}
    };

    std::vector<op> optable;
    std::vector<step> steps;
    int next_step = 0;

    virtual bool construct_optable(std::istringstream &s, int crdt_num,
                                   const std::string &type) = 0;

    void full_construct(std::string &str, int crdt_num)
    {
        std::istringstream s(str);
        int opid = 0, pid = 0;
        std::string type;
        while (s)
        {
            s >> opid >> pid >> type;
            replica_num = replica_num > pid ? replica_num : pid;
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
                s.get();
        }
    }

public:
    int replica_num = 0;

    void print()
    {
        std::cout << "op_table:\n";
        for (auto &&tmp : optable)
            std::cout << tmp.full_op << " " << tmp.effect_op << '\n';
        std::cout << "steps:\n";
        for (auto &&tmp : steps)
        {
            std::cout << (tmp.type == phase::FULL ? "FULL" : "EFFECT");
            std::cout << ' ' << tmp.opid << ' ' << tmp.pid << '\n';
        }
        std::cout << std::flush;
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

#endif  // DMCK_OP_SCRIPT_HPP
