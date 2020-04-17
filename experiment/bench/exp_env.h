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
    void start_servers()
    {
        char cmd[160];
        for (int port = BASE_PORT; port < BASE_PORT + TOTAL_SERVERS; ++port)
        {
            sprintf(cmd,"cd ../redis_test; redis-server ./6379.conf --port %d "
                        "--pidfile /var/run/redis_%d.pid --logfile ./%d.log --dbfilename %d.rdb",
                        port,port,port,port);
            system(cmd);
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

    }

    void shutdown_servers()
    {

    }

    void clean()
    {

    }
public:
    static char sudo_pwd[32];

    exp_env()
    {
        start_servers();
        construct_repl();
        set_delay();
    }

    ~exp_env()
    {
        shutdown_servers();
        remove_delay();
        clean();
    }
};


#endif //BENCH_EXP_ENV_H
