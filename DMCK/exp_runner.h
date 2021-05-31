//
// Created by admin on 2021/5/31.
//

#ifndef DMCK_EXP_RUNNER_H
#define DMCK_EXP_RUNNER_H

#include <fstream>

#include "op_script.h"
#include "oracle.h"

class exp_env
{
private:
    static constexpr auto MAX_ROUND = 100;

    static constexpr auto IP = "127.0.0.1";
    static constexpr auto BASE_PORT = 6379;

    int round{0};
    std::vector<redis_connect> conn;

    void construct_conn(int replica_num)
    {
        if (!conn.empty())
        {
            conn.clear();
            wait_system();
        }
        for (int i = 0; i < replica_num; i++)
            conn.emplace_back(IP, BASE_PORT + i, replica_num, i);
    }

public:
    exp_env(int replica_num) { construct_conn(replica_num); }

    int round() { return round; }

    std::vector<redis_connect>& get()
    {
        if (round == MAX_ROUND)
        {
            construct_conn(conn.size());
            round = 0;
        }
        round++;
        return conn;
    }
};

static void run(const std::string& filename)
{
    std::ifstream file(filename);
    std::string operations, states;
    while (file && std::getline(file, operations))
    {
        getline(file, states);
        // TODO template ?extends...
    }
}

#endif  // DMCK_EXP_RUNNER_H
