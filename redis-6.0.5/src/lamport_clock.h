//
// Created by user on 19-10-30.
//

#ifndef REDIS_LAMPORT_CLOCK_H
#define REDIS_LAMPORT_CLOCK_H

#include "vector_clock.h"

typedef struct lamport_clock
{
    int x;
    int id;
} lc;

static inline lc *_lc_new(int x, int id)
{
    lc *t = zmalloc(sizeof(lc));
    t->id = id;
    t->x = x;
    return t;
}

#define lc_new(x) _lc_new(x, CURRENT_PID)

static inline void lc_delete(lc *t) { zfree(t); }

// used as unique tags, only used to be total ordered, like in add-win CRPQ, no update lc,
// coexisting tags == concurrent, it makes more sense to prioritise id
static inline int lc_cmp_as_tag(lc *t1, lc *t2)
{
    if (t1->id != t2->id) return t1->id - t2->id;
    return t1->x - t2->x;
}

// lamport clock compare. require update lc every time a new lc arrives.
// hence prioritize x, preserves causality
static inline int lc_cmp(const lc *t1, const lc *t2)
{
    if (t1->x != t2->x) return t1->x - t2->x;
    return t1->id - t2->id;
}

// duplicate
static inline lc *lc_dup(lc *t) { return _lc_new(t->x, t->id); }

static inline sds lcToSds(const lc *t) { return sdscatprintf(sdsempty(), "%d,%d", t->x, t->id); }

static inline lc *sdsToLc(sds s)
{
    lc *t = zmalloc(sizeof(lc));
    sscanf(s, "%d,%d", &t->x, &t->id);
    return t;
}

static inline void lc_update(lc *tar, const lc *m) { tar->x = (tar->x > m->x) ? tar->x : m->x; }

#define lc_copy(tar, m)        \
    do                         \
    {                          \
        if ((tar) == NULL)     \
            (tar) = lc_dup(m); \
        else                   \
            _lc_copy(tar, m);  \
    } while (0)

static inline void _lc_copy(lc *tar, const lc *m)
{
    tar->x = m->x;
    tar->id = m->id;
}

// get the next lc in sds format, doesn't change the current lc
static inline sds lc_now(lc *t)
{
    t->x++;
    sds rtn = lcToSds(t);
    t->x--;
    return rtn;
}

#endif  // REDIS_LAMPORT_CLOCK_H
