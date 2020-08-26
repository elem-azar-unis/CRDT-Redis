#include "exp_env.h"
#include "list/list_exp.h"
#include "rpq/rpq_exp.h"

using namespace std;

/*
const char *ips[] = {"192.168.188.135",
                      "192.168.188.136",
                      "192.168.188.137"};

void time_max()
{
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == nullptr || c->err)
    {
        if (c)
        {
            printf("Error: %s\n", c->errstr);
        }
        else
        {
            printf("Can't allocate redis context\n");
        }
        return;
    }
    struct timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);
    redisReply *reply;
    thread threads[THREAD_PER_SERVER];
    reply = static_cast<redisReply *>(redisCommand(c, "VCNEW s"));
    freeReplyObject(reply);

//    default_random_engine e;
//    uniform_int_distribution<unsigned> u(0, 4);

    for (thread &t : threads)
    {
        t = thread([] {
            redisContext *cl = redisConnect("127.0.0.1", 6379);
            redisReply *r;
            for (int i = 0; i < 10000; i++)
            {
                r = static_cast<redisReply *>(redisCommand(cl, "VCINC s"));
                freeReplyObject(r);
            }
            redisFree(cl);
        });
    }
    for (thread &t : threads)
    {
        t.join();
    }

    reply = static_cast<redisReply *>(redisCommand(c, "VCGET s"));
    printf("%s\n", reply->str);
    freeReplyObject(reply);
    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (double) (t2.tv_usec - t1.tv_usec) / 1000000;
    printf("%f, %f\n", time_diff_sec, (2.0 + THREAD_PER_SERVER * 10000) / time_diff_sec);
    redisFree(c);
}

void conn_one_server(const char *ip, const int port, vector<thread *> &thds, rpq_generator &gen)
{
    for (int i = 0; i < THREAD_PER_SERVER; ++i)
    {
        auto t = new thread([ip, port, &thds, &gen] {
            redisContext *c = redisConnect(ip, port);
            for (int times = 0; times < OP_PER_THREAD; ++times)
            {
                gen.gen_and_exec(c);
                this_thread::sleep_for(chrono::microseconds((int) SLEEP_TIME));
            }
            redisFree(c);
        });
        thds.emplace_back(t);
    }
}

void test_local(rpq_type zt)
{
    vector<thread *> thds;
    rpq_log qlog;
    rpq_generator gen(zt, qlog);

    for (int i = 0; i < 5; ++i)
    {
        conn_one_server("127.0.0.1", 6379 + i, thds, gen);
    }

    bool mb = true, ob = true;
    thread max([&mb, &qlog, zt] {
        rpq_cmd c(zt, zmax, -1, -1, qlog);
        redisContext *cl = redisConnect("127.0.0.1", 6379);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfor-loop-analysis"
        while (mb)
        {
            this_thread::sleep_for(chrono::seconds(TIME_READ));
            c.exec(cl);
        }
#pragma clang diagnostic pop
        redisFree(cl);
    });
    thread overhead([&ob, &qlog, zt] {
        rpq_cmd c(zt, zoverhead, -1, -1, qlog);
        redisContext *cl = redisConnect("127.0.0.1", 6381);
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfor-loop-analysis"
        while (ob)
        {
            this_thread::sleep_for(chrono::seconds(TIME_OVERHEAD));
            c.exec(cl);
        }
#pragma clang diagnostic pop
        redisFree(cl);
    });
    for (auto t:thds)
        t->join();
    mb = false;
    ob = false;
    printf("ending.\n");
    max.join();
    overhead.join();
    gen.stop_and_join();
    qlog.write_logfiles(rpq_type_str[zt]);
}

void test_count_dis_one(const char *ip, const int port, rpq_type zt)
{
    thread threads[THREAD_PER_SERVER];
    rpq_log qlog;
    rpq_generator gen(zt, qlog);

    timeval t1{}, t2{};
    gettimeofday(&t1, nullptr);

    for (thread &t : threads)
    {
        t = thread([&ip, &port, &gen] {
            redisContext *c = redisConnect(ip, port);
            for (int times = 0; times < 20000; ++times)
            {
                gen.gen_and_exec(c);
            }
            redisFree(c);
        });
    }

    for (thread &t : threads)
    {
        t.join();
    }

    gettimeofday(&t2, nullptr);
    double time_diff_sec = (t2.tv_sec - t1.tv_sec) + (double) (t2.tv_usec - t1.tv_usec) / 1000000;
    printf("%f, %f\n", time_diff_sec, 20000 * THREAD_PER_SERVER / time_diff_sec);

    redisContext *cl = redisConnect(ips[1], port);
    auto r = static_cast<redisReply *>(redisCommand(cl, "ozopcount"));
    printf("%lli\n", r->integer);
    freeReplyObject(r);
    redisFree(cl);
}
*/

int main(int argc, char *argv[])
{
    // time_max();
    // test_count_dis_one(ips[0],6379);

    istream::sync_with_stdio(false);
    ostream::sync_with_stdio(false);

    if (argc == 2)
        exp_env::sudo_pwd = argv[1];
    else if (argc == 1)
    {
        cout << "please enter the password for sudo: ";
        cin >> exp_env::sudo_pwd;
        cout << "\n";
    }
    else
    {
        cout << "error. too many input arguments." << endl;
        return -1;
    }

    rpq_exp().exp_start_all(30);

    return 0;
}
