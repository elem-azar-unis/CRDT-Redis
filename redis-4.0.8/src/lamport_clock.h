//
// Created by user on 19-10-30.
//

#ifndef REDIS_4_0_8_LAMPORT_CLOCK_H
#define REDIS_4_0_8_LAMPORT_CLOCK_H

#include "vector_clock.h"

typedef struct lamport_clock
{
    int x;
    int id;
} lc;

inline lc *_lc_new(int x, int id)
{
    lc *t = zmalloc(sizeof(lc));
    t->id = id;
    t->x = x;
    return t;
}

#define LC_NEW(x) _lc_new(x, CURRENT_PID)

// used as unique tags, only used to be total ordered, like in add-win CRPQ, no update lc,
// coexisting tags == concurrent, it makes more sense to prioritise id
inline int lc_cmp_as_tag(lc *t1, lc *t2)
{
    if (t1->id != t2->id)return t1->id - t2->id;
    return t1->x - t2->x;
}

// lamport clock compare. require update lc every time a new lc arrives.
// hence prioritize x, preserves causality
inline int lc_cmp(const lc *t1, const lc *t2)
{
    if (t1->x != t2->x)return t1->x - t2->x;
    return t1->id - t2->id;
}

inline lc *lc_dup(lc *t)
{
    lc *n = zmalloc(sizeof(lc));
    n->x = t->x;
    n->id = t->id;
    return n;
}

inline sds lcToSds(const lc *t)
{
    return sdscatprintf(sdsempty(), "%d,%d", t->x, t->id);
}

inline lc *sdsToLc(sds s)
{
    lc *t = zmalloc(sizeof(lc));
    sscanf(s, "%d,%d", &t->x, &t->id);
    return t;
}

inline lc *lc_update(lc *tar, const lc *m)
{
    tar->x = (tar->x > m->x) ? tar->x : m->x;
}
#define LC_COPY(tar,m)\
do\
{\
    if((tar)==NULL)\
        (tar)=lc_dup(m);\
    else\
    {\
        (tar)->x = (m)->x;\
        (tar)->id = (m)->id;\
    }\
}while(0)
inline void *lc_copy(lc *tar, const lc *m)
{
    tar->x = m->x;
    tar->id = m->id;
}

// get the next lc in sds format, doesn't change the current lc
inline sds lc_now(lc *t)
{
    t->x++;
    sds rtn = lcToSds(t);
    t->x--;
    return rtn;
}

#endif //REDIS_4_0_8_LAMPORT_CLOCK_H
