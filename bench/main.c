#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <hiredis/hiredis.h>
#define TC 5
void* test(void* threadid)
{
    redisContext *c = redisConnect("127.0.0.1", 6379);
    redisReply* reply;
    for(int i=0;i<10000;i++)
    {
        reply = redisCommand(c, "VCINC s");
        freeReplyObject(reply);
    }
}
int main(int argc, char *argv[])
{
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Error: %s\n", c->errstr);
        } else {
            printf("Can't allocate redis context\n");
        }
        return -1;
    }
    double ti=clock();
    redisReply* reply;
    pthread_t threads[TC];
    reply = redisCommand(c, "VCNEW s");
    freeReplyObject(reply);

    for(int i=0;i<TC;i++)
    {
        pthread_create(&threads[i],NULL,test,NULL);
    }
    for(int i=0;i<TC;i++)
    {
        pthread_join(threads[i],NULL);
    }

    reply = redisCommand(c, "VCGET s");
    printf("%s\n",reply->str);
    freeReplyObject(reply);
    ti=(clock()-ti)/CLOCKS_PER_SEC;
    printf("%f, %f\n",ti,(2.0+TC*10000)/ti);
    redisFree(c);
    return 0;
}