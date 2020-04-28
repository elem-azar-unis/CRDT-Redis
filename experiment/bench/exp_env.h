//
// Created by admin on 2020/4/16.
//

#ifndef BENCH_EXP_ENV_H
#define BENCH_EXP_ENV_H

#include <cstdio>
#include <cstring>
#include <chrono>
#include <thread>

#include "constants.h"

#define IP_BETWEEN_CLUSTER "127.0.0.3"
#define IP_WITHIN_CLUSTER "127.0.0.2"
#define IP_SERVER "127.0.0.1"
#define BASE_PORT 6379

#define SUDO_PREFIX "echo %s | sudo -S "
#define CMD_SUFFIX " 1>/dev/null"   // or " 1>/dev/null 2>&1"

class exp_env
{
private:
    static void shell_exec(const char *cmd, bool sudo)
    {
//#define PRINT_CMD
#ifdef PRINT_CMD
        if (sudo)
            printf("\nsudo %s\n", cmd);
        else
            printf("\n%s\n", cmd);
#endif
        char *_cmd_apend;
        if (sudo)
        {
            _cmd_apend = new char[strlen(cmd) + strlen(SUDO_PREFIX CMD_SUFFIX) + 1];
            sprintf(_cmd_apend, SUDO_PREFIX "%s" CMD_SUFFIX, sudo_pwd, cmd);
        }
        else
        {
            _cmd_apend = new char[strlen(cmd) + strlen(CMD_SUFFIX) + 1];
            sprintf(_cmd_apend, "%s" CMD_SUFFIX, cmd);
        }
        system(_cmd_apend);
        delete[] _cmd_apend;
    }

    static void start_servers()
    {
        char cmd[128];
        for (int port = BASE_PORT; port < BASE_PORT + TOTAL_SERVERS; ++port)
        {
            sprintf(cmd, "redis-server ../redis_test/6379.conf "
                         "--port %d --logfile %d.log "
                         "--pidfile /var/run/redis_%d.pid "
                         "--dbfilename %d.rdb",
                    port, port, port, port);
            shell_exec(cmd, false);
        }
    }

    static void construct_repl()
    {
        char *cmd = new char[64 + 16 * (TOTAL_SERVERS - 1)];
        char *repl = new char[16 * (TOTAL_SERVERS - 1)];
        for (int i = 0; i < exp_setting::total_clusters; ++i)
        {
            repl[0] = '\0';
            for (int k = 0; k < i * exp_setting::total_clusters; ++k)
                sprintf(repl, "%s " IP_BETWEEN_CLUSTER " %d", repl, BASE_PORT + k);
            for (int j = 0; j < exp_setting::server_per_cluster; ++j)
            {
                int num = i * exp_setting::total_clusters + j;
                sprintf(cmd, "redis-cli -h 127.0.0.1 -p %d "
                             "REPLICATE %d %d exp_local%s",
                        BASE_PORT + num, TOTAL_SERVERS, num, repl);
                shell_exec(cmd, false);
                sprintf(repl, "%s " IP_WITHIN_CLUSTER " %d", repl, BASE_PORT + num);
            }
        }
        delete[] cmd;
        delete[] repl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    static void set_delay()
    {
        char cmd[128];
        shell_exec("tc qdisc add dev lo root handle 1: prio", true);

        sprintf(cmd, "tc qdisc add dev lo parent 1:1 handle 10: "
                     "netem delay %dms %dms distribution normal limit 100000",
                exp_setting::delay_low, exp_setting::delay_low / 5);
        shell_exec(cmd, true);
        shell_exec("tc filter add dev lo protocol ip parent 1: prio 1 u32 match ip dst "
                   IP_WITHIN_CLUSTER " flowid 1:1", true);

        sprintf(cmd, "tc qdisc add dev lo parent 1:2 handle 20: "
                     "netem delay %dms %dms distribution normal limit 100000",
                exp_setting::delay, exp_setting::delay / 5);
        shell_exec(cmd, true);
        shell_exec("tc filter add dev lo protocol ip parent 1: prio 1 u32 match ip dst "
                   IP_BETWEEN_CLUSTER " flowid 1:2", true);

        shell_exec("tc qdisc add dev lo parent 1:3 handle 30: pfifo_fast", true);
    }

    static void remove_delay()
    {
        shell_exec("tc filter del dev lo", true);
        shell_exec("tc qdisc del dev lo root", true);
    }

    static void shutdown_servers()
    {
        char cmd[64];
        for (int port = BASE_PORT; port < BASE_PORT + TOTAL_SERVERS; ++port)
        {
            sprintf(cmd, "redis-cli -h 127.0.0.1 -p %d SHUTDOWN NOSAVE", port);
            shell_exec(cmd, false);
        }
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    static void clean()
    {
        shell_exec("rm -rf *.rdb *.log", false);
    }

public:
    static char sudo_pwd[32];

    exp_env()
    {
        exp_setting::print_settings();
        start_servers();
        printf("server started, ");
        construct_repl();
        printf("replication constructed, ");
        set_delay();
        printf("delay set\n");
    }

    ~exp_env()
    {
        remove_delay();
        printf("delay removed, ");
        shutdown_servers();
        printf("server shutdown, ");
        clean();
        printf("cleaned\n\n");
    }
};


#endif //BENCH_EXP_ENV_H
