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
 * Precondition check for the prepare part.
 * Add operation and non-add operations.
 * e is the metadata element.
 * */
#define PREPARE_PRECOND_ADD(e) \
do\
{\
    if (EXISTS(e))\
    {\
        addReply(c, shared.ele_exist);\
        return;\
    }\
}while(0)

#define PREPARE_PRECOND_NON_ADD(e) \
do\
{\
    if (e == NULL || !EXISTS(e))\
    {\
        addReply(c, shared.ele_nexist);\
        return;\
    }\
}while(0)

/*
 * Three functions to check if it is ready for increase/remove/update.
 * Do the corresponding check before you do the actual effect phase.
 * */

inline int insertCheck(reh *h, vc *t)
{
    if (equalVC(t, CURRENT(h)) == 0)return 0;
    return PID(h) < t->id;
}

inline int updateCheck(reh *h, vc *t)
{
    return equalVC(t, CURRENT(h));
}

inline int removeCheck(reh *h, vc *t)
{
    for (int i = 0; i < t->size; ++i)
        if (CURRENT(h)->vector[i] < t->vector[i])
            return 1;
    return 0;
}


/* Get the hash table -- the top layer of the container metadata.
 *
 * redisDb *db : the data base
 * sds tname : the name of the container
 * const char *suffix : the suffix of the container, can be null string
 * int create : 1 (or 0) if you want to create the container if it doesn't exist (or not)
 * */
robj *getInnerHT(redisDb *db, robj* tname, const char *suffix, int create);

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
 *
 * Note that you may wrap this function with a macro for the convenience of use. For example,
 * in remove-win CRPQ, the element type name is rze:
 *
 * #ifndef RW_OVERHEAD
 * #define GET_RZE(arg_t, create)\
 * (rze *) rehHTGet(c->db, c->arg_t[1], RW_RPQ_TABLE_SUFFIX, c->arg_t[2], create, rzeNew)
 * #else
 * #define RZE_HT_GET(arg_t,create)\
 * (rze *) rehHTGet(c->db, c->arg_t[1], RW_RPQ_TABLE_SUFFIX, c->arg_t[2], create, rzeNew, cur_db, cur_tname, SUF_RZETOTAL)
 * #endif
 *
 * */
reh *rehHTGet(redisDb *db, robj *tname, const char *suffix, robj *key, int create, reh *(*p)()
#ifdef RW_OVERHEAD
        ,redisDb *cur_db, sds cur_tname, const char *ovhd_suf
#endif
);

// set key(sds type) with value(value_t type) in ht(hash table type)
#define RWFHT_SET(ht, key, value_t, value) \
hashTypeSet(ht, key, sdsnewlen(&(value), sizeof(value_t)), HASH_SET_TAKE_VALUE)

/*
 * Add the remove history into rargv. Use the macro at the end of the prepare phase.
 * Suppose the target element you get from the CRDC (or may be newly created) is e, then choose the macro:
 * - non-remove operations: ADD_CR_NON_RMV(e)
 * - remove operation: ADD_CR_RMV(e)
 * */
#define ADD_CR_NON_RMV(e) RARGV_ADD_SDS(VCToSds(CURRENT(e)))
#define ADD_CR_RMV(e) RARGV_ADD_SDS(nowVC(CURRENT((reh *) (e))))

// Get the timestamp from rargv. Remember to free it when it's no longer needed.
#define CR_GET SdsToVC(c->rargv[c->rargc-1]->ptr)

#endif //RWFRAMEWORK_H
