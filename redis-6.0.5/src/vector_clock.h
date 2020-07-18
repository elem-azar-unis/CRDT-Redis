//
// Created by user on 18-6-21.
//

#ifndef REDIS_VECTOR_CLOCK_H
#define REDIS_VECTOR_CLOCK_H
#include "sds.h"

typedef struct vector_clock
{
    int *vector;
    int size;
    int id;
} vc;

#define VC_LESS (-2)
#define VC_C_LESS (-1)
#define VC_CMP_ERR 0
#define VC_C_GREATER 1
#define VC_GREATER 2

#define CONCURRENT(x) ((x) == VC_C_LESS || (x) == VC_C_GREATER)

#define VC_SIZE (sizeof(vc) + server.p2p_count * sizeof(int))
#define CURRENT_PID server.p2p_id
#define vc_new() _vc_new(server.p2p_count, CURRENT_PID)
#define vc_inc(c) _vc_inc((c), CURRENT_PID)

vc *_vc_new(int size, int id);
// duplicate
vc *vc_dup(const vc *c);
int vc_cmp(const vc *c1, const vc *c2);
vc *vc_update(vc *tar, const vc *m);
sds vcToSds(const vc *c);
vc *sdsToVC(sds s);
int vc_equal(const vc *c1, const vc *c2);
int vc_causally_ready(const vc *current, const vc *next);
static inline void vc_delete(vc *c)
{
    zfree(c->vector);
    zfree(c);
}

static inline vc *_vc_inc(vc *c, int id)
{
    c->id = id;
    c->vector[id]++;
    return c;
}

// get the next vc in sds format, doesn't change the current vc
static inline sds vc_now(vc *c)
{
    c->vector[c->id]++;
    sds rtn = vcToSds(c);
    c->vector[c->id]--;
    return rtn;
}

#endif  // REDIS_VECTOR_CLOCK_H
