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
    explicit exp_env(int replica_num) { construct_conn(replica_num); }

    int get_round() const { return round; }

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

template <
    typename S, typename = typename std::enable_if<std::is_base_of<op_script, S>::value, S>::type,
    typename O, typename = typename std::enable_if<std::is_base_of<oracle, O>::value, O>::type>
static void run(const std::string& filename)
{
    std::ifstream file(filename);
    std::string operations, states;

    int replica_num{0};
    file >> replica_num;
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    exp_env env{replica_num};

    while (file && std::getline(file, operations))
    {
        auto& conn = env.get();

        S sc{operations, env.get_round()};
        sc.run(conn);

        getline(file, states);
        O orcl{states};
        if (!orcl.check(conn, env.get_round()))
        {
            std::cout << "Check failed!\n";
            sc.print();
            orcl.print();
        }
    }
}

#endif  // DMCK_EXP_RUNNER_H
