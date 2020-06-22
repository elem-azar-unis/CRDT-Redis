//
// Created by yqzhang on 2020/6/20.
//

#ifndef REDIS_4_0_8_CRDT_EXP_H
#define REDIS_4_0_8_CRDT_EXP_H

//#define CRDT_EXPERIMENT
#ifdef CRDT_EXPERIMENT
#define CRDT_LOG
#define CRDT_OVERHEAD
#define CRDT_OPCOUNT
#define CRDT_ELE_STATUS
#endif

#ifdef CRDT_LOG
#include <sys/time.h>
long currentTime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return  tv.tv_sec*1000000+tv.tv_usec;
}
#endif

#ifdef CRDT_OVERHEAD
/*
#define PRE_SET \
do{\
   cur_db=c->db;cur_tname=c->argv[1]->ptr;\
}while(0)
robj* _get_ovhd_count(redisDb* db,sds tname,const char* suf);
void inc_ovhd_count(redisDb* db,sds tname,const char* suf,long i);
long get_ovhd_count(redisDb* db,sds tname,const char* suf);
*/
void ozoverheadCommand(client* c);
void rzoverheadCommand(client* c);
void rwfzoverheadCommand(client* c);
#endif

#ifdef CRDT_OPCOUNT
void ozopcountCommand(client *c);
void rzopcountCommand(client *c);
#endif

#endif //REDIS_4_0_8_CRDT_EXP_H
