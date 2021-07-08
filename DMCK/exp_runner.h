//
// Created by admin on 2021/5/31.
//

#ifndef DMCK_EXP_RUNNER_H
#define DMCK_EXP_RUNNER_H

#include <cstdlib>
#include <fstream>

#include "op_script.h"
#include "oracle.h"
#include "timer.h"

class exp_env
{
private:
    static constexpr auto MAX_ROUND = 50000;

    static constexpr auto IP = "127.0.0.1";
    static constexpr auto BASE_PORT = 6379;

    int round{0}, loop{1};
    std::vector<redis_connect> conn;

    void construct_conn(int replica_num)
    {
        if (!conn.empty())
        {
            conn.clear();
            wait_system();
        }
        for (size_t i = 0; i < replica_num; i++)
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

            std::cout << "Passed " << loop * MAX_ROUND << " scripts." << std::endl;
            loop++;
        }
        round++;
        return conn;
    }
};

template <typename Scirpt, typename Oracle>
static std::enable_if_t<std::is_base_of_v<op_script, Scirpt> && std::is_base_of_v<oracle, Oracle>>
run(std::string_view filename, bool verbose = false)
{
    timer time;

    std::ifstream file(filename.data());
    if (!file)
    {
        std::cout << "--Error: No such file \"" << filename << "\"!" << std::endl;
        exit(-1);
    }

    std::cout << "Start checking for script \"" << filename << "\"." << std::endl;

    int replica_num{0};
    file >> replica_num;
    file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    exp_env env{replica_num};

    std::string operations, states;
    int count = 0;
    while (file && std::getline(file, operations))
    {
        auto& conn = env.get();

        Scirpt sc{operations, env.get_round(), verbose};
        sc.run(conn);

        getline(file, states);
        Oracle orcl{states};
        if (!orcl.check(conn, env.get_round()))
        {
            std::cout << "Check failed for test No." << count + 1 << "! Line " << 2 + 2 * count
                      << " in the file.\n";
            sc.print();
            orcl.print();
            return;
        }
        count++;
    }

    std::cout << "All " << count << " script checks passed!" << std::endl;
}

#endif  // DMCK_EXP_RUNNER_H
