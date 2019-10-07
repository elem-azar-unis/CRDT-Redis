//
// Created by user on 19-10-4.
//

#ifndef RWFRAMEWORK_H
#define RWFRAMEWORK_H

#include "server.h"

/*
 * The header of elements' metadata.
 * The construct macro.
 * The remove function macro of header.
 *
 * For the header, put it at the top of your actual struct to 'inherit' it.
 * e.g.
 * typedef struct RW_RPQ_element
 * {
 *     reh header;
 *     double innate;
 *     double acquired;
 * } rze;
 *
 * And add the header construct macro to your actual construct function.
 * e.g.
 * rze *rzeNew()
 * {
 *     rze *e = zmalloc(sizeof(rze));
 *     REH_INIT((reh *) e);
 *     e->innate = 0;
 *     e->acquired = 0;
 *     return e;
 * }
 *
 * And add the remove macro to your actual element remove effect function.
 * e.g.
 * void removeFunc(client *c, rze *e, vc *t)
 * {
 *     if (removeCheck(e, t))
 *     {
 *         REH_RMV_FUNC(e,t);
 *         e->acquired = 0;
 *         e->innate = 0;
 *         robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
 *         zsetDel(zset, c->rargv[2]->ptr);
 *         server.dirty++;
 *     }
 * }
 * */

typedef struct RW_element_header
{
    int pid;
    vc *current;
} reh;

// The access macro for fields in header.
#define PID(h) (((reh*)(h))->pid)
#define CURRENT(h) (((reh*)(h))->current)

#define REH_INIT(h) \
do\
{\
    CURRENT(h) = l_newVC;\
    PID(h) = -1;\
}while(0)

#define REH_RMV_FUNC(h, t) \
do\
{\
    updateVC(CURRENT(h), t);\
    PID(h) = -1;\
}while(0)

// If the element is in the container.
#define EXISTS(h) (PID(h) >= 0)

/*
 * Three functions to check if it is ready for increase/remove/update.
 * Do the corresponding check before you do the actual effect phase.
 * */

inline int insertCheck(reh *h, vc *t)
{
    if (equalVC(t, h->current) == 0)return 0;
    return h->pid < t->id;
}

inline int updateCheck(reh *h, vc *t)
{
    return equalVC(t, h->current);
}

inline int removeCheck(reh *h, vc *t)
{
    for (int i = 0; i < t->size; ++i)
        if (h->current->vector[i] < t->vector[i])
            return 1;
    return 0;
}

// Get the current timestamp for the element in sds format.
inline sds now(reh *h)
{
    CURRENT(h)->vector[CURRENT(h)->id]++;
    sds rtn = VCToSds(CURRENT(h));
    CURRENT(h)->vector[CURRENT(h)->id]--;
    return rtn;
}

/* Get the hash table -- the top layer of the container metadata.
 *
 * redisDb *db : the data base
 * sds tname : the name of the container
 * const char *suffix : the suffix of the container, can be null string
 * int create : 1 (or 0) if you want to create the container if it doesn't exist (or not)
 * */
robj *getInnerHT(redisDb *db, sds tname, const char *suffix, int create);

/* Get the element metadata type from the container, in the format of the header struct. You may cast it
 * to your actual element struct.
 *
 * redisDb *db : the data base
 * robj *tname : the name of the container
 * const char *suffix : the suffix of the container metadata
 * robj *key : the key of the element
 * int create : 1 (or 0) if you want to create the container and the element if they don't exist (or not)
 * reh* (*p)() : The pointer to the create function of the element struct
 *
 * If define RW_OVERHEAD, count the memory usage when creating elements.
 * */
reh *rehHTGet(redisDb *db, robj *tname, const char *suffix, robj *key, int create, reh *(*p)()
#ifdef RW_OVERHEAD
        ,redisDb *cur_db, sds cur_tname, const char *ovhd_suf
#endif
);

/*
 * Prepare rargc and rargv for RW-CRDCs.
 * - copy the current argv to rargv
 * - add the current timestamp cr to the end of rargv
 *
 * cr : the timestamp. Suppose the target element you get from the CRDC (or may be newly created) is e, then cr is:
 * - non-remove operations: CR_NON_RMV(e)
 * - remove operation: CR_RMV(e)
 * */
#define RWF_RARG_PREPARE(cr) \
do\
{\
    PREPARE_RARGC(c->argc + 1);\
    for(int _i_=0; _i_ < c->argc; _i_++)\
        COPY_ARG_TO_RARG(_i_, _i_);\
    c->rargv[c->argc] = createObject(OBJ_STRING, cr);\
}while(0);

#define CR_NON_RMV(e) VCToSds(CURRENT(e))
#define CR_RMV(e) now((reh *) (e))

// Get the timestamp from rargv. Remember to free it when it's no longer needed.
#define CR_GET SdsToVC(c->rargv[c->rargc-1]->ptr)

#endif //RWFRAMEWORK_H
