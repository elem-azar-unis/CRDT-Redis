//
// Created by yqzhang on 2020/6/20.
//

#ifndef REDIS_4_0_8_CRDT_EXP_H
#define REDIS_4_0_8_CRDT_EXP_H

#include "server.h"

//#define CRDT_EXPERIMENT
#ifdef CRDT_EXPERIMENT
//#define CRDT_LOG
#define CRDT_OVERHEAD
#define CRDT_OPCOUNT
//#define CRDT_ELE_STATUS
#endif

#ifdef CRDT_LOG

#include <sys/time.h>

static FILE *__CRDT_log = NULL;

static inline long currentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

#define CHECK_FILE                               \
    do                                           \
    {                                            \
        if (__CRDT_log == NULL)                  \
            __CRDT_log = fopen("crdt.log", "a"); \
    } while (0)

#define LOG_ISTR_PRE                                               \
    do                                                             \
    {                                                              \
        CHECK_FILE;                                                \
        fprintf(__CRDT_log, "%ld, PREPARE:", currentTime());       \
        for (int i = 0; i < c->argc; i++)                          \
        {                                                          \
            fprintf(__CRDT_log, " %s", (char *)(c->argv[i]->ptr)); \
        }                                                          \
        fprintf(__CRDT_log, "\n");                                 \
        fflush(__CRDT_log);                                        \
    } while (0)

#define LOG_ISTR_EFF                                                \
    do                                                              \
    {                                                               \
        CHECK_FILE;                                                 \
        fprintf(__CRDT_log, "%ld, EFFECT:", currentTime());         \
        for (int i = 0; i < c->rargc; i++)                          \
        {                                                           \
            fprintf(__CRDT_log, " %s", (char *)(c->rargv[i]->ptr)); \
        }                                                           \
        fprintf(__CRDT_log, "\n");                                  \
        fflush(__CRDT_log);                                         \
    } while (0)

void CRDT_log(const char *fmt, ...)
{
    va_list ap;
    char msg[LOG_MAX_LEN];

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    CHECK_FILE;
    fprintf(__CRDT_log, "%ld, user_log: %s\n", currentTime(), msg);
    fflush(__CRDT_log);
}

#else
#define LOG_ISTR_PRE
#define LOG_ISTR_EFF
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

void ozoverheadCommand(client *c);
void rzoverheadCommand(client *c);
void rwfzoverheadCommand(client *c);
#endif

#ifdef CRDT_OPCOUNT

static int __op_count = 0;
#define OPCOUNT_ISTR __op_count++;
static inline int get_op_count()
{
    return __op_count;
}

void ozopcountCommand(client *c);
void rzopcountCommand(client *c);
void rwfzopcountCommand(client *c);
void rlopcountCommand(client *c);
void rwflopcountCommand(client *c);

#else
#define OPCOUNT_ISTR
#endif

#ifdef CRDT_ELE_STATUS
void ozestatusCommand(client *c);
void rzestatusCommand(client *c);
void rwfzestatusCommand(client *c);
#endif

#endif //REDIS_4_0_8_CRDT_EXP_H
