//
// Created by admin on 2020/4/16.
//

#ifndef BENCH_EXP_ENV_H
#define BENCH_EXP_ENV_H

#include <chrono>
#include <sstream>
#include <thread>

#include "exp_setting.h"

#define REDIS_SERVER "../../redis-6.0.5/src/redis-server"
#define REDIS_CONF "../../redis-6.0.5/redis.conf"
#define REDIS_CLIENT "../../redis-6.0.5/src/redis-cli"

#define IP_BETWEEN_CLUSTER "127.0.0.3"
#define IP_WITHIN_CLUSTER "127.0.0.2"
#define IP_SERVER "127.0.0.1"
constexpr int BASE_PORT = 6379;

class exp_env
{
private:
    static void shell_exec(const char* cmd, bool sudo)
    {
//#define PRINT_CMD
#ifdef PRINT_CMD
        if (sudo)
            cout << "\nsudo " << cmd << endl;
        else
            cout << "\n" << cmd << endl;
#endif
        ostringstream cmd_stream;
        if (sudo) cmd_stream << "echo " << sudo_pwd << " | sudo -S ";
        cmd_stream << cmd << " 1>/dev/null";  // or " 1>/dev/null 2>&1"
        system(cmd_stream.str().c_str());
    }

    static inline void shell_exec(const string& cmd, bool sudo) { shell_exec(cmd.c_str(), sudo); }

    static void start_servers()
    {
        for (int port = BASE_PORT; port < BASE_PORT + TOTAL_SERVERS; ++port)
        {
            ostringstream stream;
            stream << REDIS_SERVER << " " << REDIS_CONF << " "
                   << "--protected-mode no "
                   << "--daemonize yes "
                   << "--loglevel debug "
                   << "--io-threads 2 "
                   << "--port " << port << " "
                   << "--logfile " << port << ".log "
                   << "--dbfilename " << port << ".rdb "
                   << "--pidfile /var/run/redis_" << port << ".pid";
            shell_exec(stream.str(), false);
        }
    }

    static void construct_repl()
    {
        for (int i = 0; i < exp_setting::total_clusters; ++i)
        {
            ostringstream repl_stream;
            for (int k = 0; k < i * exp_setting::server_per_cluster; ++k)
                repl_stream << " " << IP_BETWEEN_CLUSTER << " " << BASE_PORT + k;
            for (int j = 0; j < exp_setting::server_per_cluster; ++j)
            {
                ostringstream cmd_stream;
                int num = i * exp_setting::server_per_cluster + j;
                cmd_stream << REDIS_CLIENT << " -h 127.0.0.1 -p " << BASE_PORT + num
                           << " REPLICATE " << TOTAL_SERVERS << " " << num << " exp_local"
                           << repl_stream.str();
                shell_exec(cmd_stream.str(), false);
                repl_stream << " " << IP_WITHIN_CLUSTER << " " << BASE_PORT + num;
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    static void set_delay()
    {
        shell_exec("tc qdisc add dev lo root handle 1: prio", true);

        ostringstream stream;
        stream << "tc qdisc add dev lo parent 1:1 handle 10: netem delay " << exp_setting::delay_low
               << "ms " << exp_setting::delay_low / 5.0 << "ms distribution normal limit 100000";
        shell_exec(stream.str(), true);
        shell_exec(
            "tc filter add dev lo protocol ip parent 1: "
            "prio 1 u32 match ip dst " IP_WITHIN_CLUSTER " flowid 1:1",
            true);

        stream.str("");
        stream << "tc qdisc add dev lo parent 1:2 handle 20: netem delay " << exp_setting::delay
               << "ms " << exp_setting::delay / 5.0 << "ms distribution normal limit 100000";
        shell_exec(stream.str(), true);
        shell_exec(
            "tc filter add dev lo protocol ip parent 1: "
            "prio 1 u32 match ip dst " IP_BETWEEN_CLUSTER " flowid 1:2",
            true);

        shell_exec("tc qdisc add dev lo parent 1:3 handle 30: pfifo_fast", true);
        shell_exec(
            "tc filter add dev lo protocol ip parent 1: prio 2 u32 match ip dst 0/0 flowid 1:3",
            true);
    }

    static void remove_delay()
    {
        // shell_exec("tc filter del dev lo", true);
        shell_exec("tc qdisc del dev lo root", true);
    }

    static void shutdown_servers()
    {
        for (int port = BASE_PORT; port < BASE_PORT + TOTAL_SERVERS; ++port)
        {
            ostringstream stream;
            stream << REDIS_CLIENT << " -h 127.0.0.1 -p " << port << " SHUTDOWN NOSAVE";
            shell_exec(stream.str(), false);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    static void clean() { shell_exec("rm -rf *.rdb *.log", false); }

public:
    static string sudo_pwd;

    exp_env()
    {
        exp_setting::print_settings();
        start_servers();
        cout << "server started, " << flush;
        construct_repl();
        cout << "replication constructed, " << flush;
        set_delay();
        cout << "delay set" << endl;
    }

    ~exp_env()
    {
        remove_delay();
        cout << "delay removed, " << flush;
        shutdown_servers();
        cout << "server shutdown, " << flush;
        clean();
        cout << "cleaned\n" << endl;
    }
};

#endif  // BENCH_EXP_ENV_H
