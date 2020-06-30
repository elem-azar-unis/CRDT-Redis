//
// Created by admin on 2020/6/30.
//

#include "rpq_exp.h"
#include "rpq_generator.h"
#include "../exp_runner.h"

exp_setting::default_setting rpq_exp::rpq_setting{
    .delay = 50,
    .delay_low = 10,
    .total_clusters = 3,
    .server_per_cluster = 3,
    .op_per_sec = 10000,
    .speed_e{.start = 500, .end = 10000, .step = 100},
    .replica_e{.start = 1, .end = 5, .step = 1},
    .delay_e{.start = 20, .end = 380, .step = 1}};

void rpq_exp::test_dis(rpq_type zt)
{
    rpq_log qlog(zt);
    rpq_generator gen(zt, qlog);
    rpq_cmd read_max(zt, rpq_op_type::max, -1, -1, qlog);
    rpq_cmd ovhd(zt, rpq_op_type::overhead, -1, -1, qlog);
    rpq_cmd opcount(zt, rpq_op_type::opcount, -1, -1, qlog);

    exp_runner<int> runner(qlog, gen);
    runner.set_cmd_opcount(opcount);
    runner.set_cmd_ovhd(ovhd);
    runner.set_cmd_read(read_max);
    runner.run();
}

void rpq_exp::delay_fix(int delay, int round, rpq_type type)
{
    setting_setter tmp;
    exp_setting::set_delay(round, delay, delay / 5);
    /*
    char cmd[64];
    sprintf(cmd, "python3 ../redis_test/connection.py %d %d %d %d", delay, delay / 5, delay / 5, delay / 25);
    system(cmd);
    */
    test_dis(type);
}

void rpq_exp::test_delay(int round)
{
    for (int delay = rpq_setting.delay_e.start;
         delay <= rpq_setting.delay_e.end;
         delay += rpq_setting.delay_e.step)
    {
        delay_fix(delay, round, rpq_type::r);
        delay_fix(delay, round, rpq_type::rwf);
    }
}

void rpq_exp::replica_fix(int s_p_c, int round, rpq_type type)
{
    setting_setter tmp;
    exp_setting::set_replica(round, 3, s_p_c);
    /*
    char cmd[64];
    sprintf(cmd, "python3 ../redis_test/connection.py %d", s_p_c);
    system(cmd);
    */
    test_dis(type);
}

void rpq_exp::test_replica(int round)
{
    for (int replica = rpq_setting.replica_e.start;
         replica <= rpq_setting.replica_e.end;
         replica += rpq_setting.replica_e.step)
    {
        replica_fix(replica, round, rpq_type::r);
        replica_fix(replica, round, rpq_type::rwf);
    }
}

void rpq_exp::speed_fix(int speed, int round, rpq_type type)
{
    setting_setter tmp;
    // system("python3 ../redis_test/connection.py");
    exp_setting::set_speed(round, speed);
    test_dis(type);
}

void rpq_exp::test_speed(int round)
{
    for (int speed = rpq_setting.speed_e.start;
         speed <= rpq_setting.speed_e.end;
         speed += rpq_setting.speed_e.step)
    {
        speed_fix(speed, round, rpq_type::r);
        speed_fix(speed, round, rpq_type::rwf);
    }
}

void rpq_exp::exp_start_all(int rounds)
{
    auto start = chrono::steady_clock::now();

    setting_setter tmp;
    exp_setting::set_pattern("ardominant");
    // system("python3 ../redis_test/connection.py");
    test_dis(rpq_type::r);
    // system("python3 ../redis_test/connection.py");
    test_dis(rpq_type::rwf);

    for (int i = 0; i < rounds; i++)
    {
        test_delay(i);
        test_replica(i);
        test_speed(i);
    }

    auto end = chrono::steady_clock::now();
    auto time = chrono::duration_cast<chrono::duration<double>>(end - start).count();
    printf("total time: %f seconds\n", time);
}
