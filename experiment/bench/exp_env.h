//
// Created by admin on 2020/4/16.
//

#ifndef BENCH_EXP_ENV_H
#define BENCH_EXP_ENV_H

#include <cstdlib>
#include<chrono>
#include<thread>

#include "constants.h"

#define IP_BETWEEN_CLUSTER "127.0.0.3"
#define IP_WITHIN_CLUSTER "127.0.0.2"
#define IP_SERVER "127.0.0.1"
#define BASE_PORT 6379

class exp_env
{
private:
    void shell_exec(const char *cmd)
    {
#define PRINT_CMD
#ifdef PRINT_CMD
        printf("%s\n", cmd);
#endif
        char *_cmd_apend = new char[strlen(cmd) + 20];
        sprintf(_cmd_apend, "%s 1>/dev/null", cmd);   // or "1>/dev/null 2>&1"
        system(_cmd_apend);
        delete[] _cmd_apend;
    }

    void start_servers()
    {
        char cmd[160];
        for (int port = BASE_PORT; port < BASE_PORT + TOTAL_SERVERS; ++port)
        {
            sprintf(cmd, "cd ../redis_test; redis-server ./6379.conf --port %d "
                         "--pidfile /var/run/redis_%d.pid --logfile ./%d.log --dbfilename %d.rdb",
                    port, port, port, port);
            shell_exec(cmd);
        }
    }

    void construct_repl()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    void set_delay()
    {

    }

    void remove_delay()
    {
        "echo user | sudo -S ifconfig";
    }

    void shutdown_servers()
    {
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    void clean()
    {
        shell_exec("cd ../redis_test; rm -rf *.rdb *.log");
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
