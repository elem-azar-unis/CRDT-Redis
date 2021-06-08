//
// Created by yqzhang on 2020/6/20.
//

#ifndef REDIS_CRDT_EXP_H
#define REDIS_CRDT_EXP_H

#include "server.h"

#define CRDT_EXPERIMENT
#ifdef CRDT_EXPERIMENT

//#define CRDT_LOG
#define CRDT_OVERHEAD
//#define CRDT_OPCOUNT
#define CRDT_ELE_STATUS
//#define CRDT_TIME

#endif

#if defined(CRDT_LOG) || defined(CRDT_TIME)

#include <sys/time.h>

#include "vector_clock.h"

static inline long currentTime()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000 + tv.tv_usec;
}

static FILE *__CRDT_log = NULL;

#define CHECK_FILE                                              \
    do                                                          \
    {                                                           \
        char fname[32];                                         \
        sprintf(fname, "server%d_crdt.log", CURRENT_PID);       \
        if (__CRDT_log == NULL) __CRDT_log = fopen(fname, "a"); \
    } while (0)

static void CRDT_log(const char *fmt, ...)
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

#endif

#ifdef CRDT_TIME

#define TIME_ISTR_BEGIN long __begin__ = currentTime();

#define TIME_ISTR_END CRDT_log("%s, %d: %ld", c->argv[0]->ptr, c->argc, currentTime() - __begin__);

#else

#define TIME_ISTR_BEGIN
#define TIME_ISTR_END

#endif

#ifdef CRDT_LOG

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
void inc_ovhd_count(redisDb* db,sds tname,const char* suf,long i);
long get_ovhd_count(redisDb* db,sds tname,const char* suf);
*/

static long __mem_ovhd = 0;
static inline void ovhd_inc(long inc) { __mem_ovhd += inc; }
static inline long ovhd_get() { return __mem_ovhd; }

void ozoverheadCommand(client *c);
void rzoverheadCommand(client *c);
void rwfzoverheadCommand(client *c);
void rloverheadCommand(client *c);
void rwfloverheadCommand(client *c);
#endif

#ifdef CRDT_OPCOUNT

static int __op_count = 0;
#define OPCOUNT_ISTR __op_count++;
static inline int op_count_get() { return __op_count; }

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

#endif  // REDIS_CRDT_EXP_H
