#include <cstdio>
#include <ctime>
#include <thread>
#include <random>
#include <hiredis/hiredis.h>

#include "constants.h"

using namespace std;

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
    double ti = clock();
    redisReply *reply;
    thread threads[THREAD_PER_SERVER];
    reply = static_cast<redisReply *>(redisCommand(c, "VCNEW s"));
    freeReplyObject(reply);

    default_random_engine e;
    uniform_int_distribution<unsigned> u(0, 4);

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
    ti = (clock() - ti) / CLOCKS_PER_SEC;
    printf("%f, %f\n", ti, (2.0 + THREAD_PER_SERVER * 10000) / ti);
    redisFree(c);
}

int main(int argc, char *argv[])
{
    //time_max();
    redisContext *c = redisConnect("127.0.0.1", 6379);
    auto reply = static_cast<redisReply *>(redisCommand(c, "ozmax s"));
    auto r=reply->element[1];
    printf("%d ",r->type);
    freeReplyObject(reply);
    redisFree(c);
    return 0;
}