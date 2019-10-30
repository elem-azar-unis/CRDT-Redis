//
// Created by user on 18-6-21.
//

#ifndef REDIS_4_0_8_VECTOR_CLOCK_H
#define REDIS_4_0_8_VECTOR_CLOCK_H
#include "sds.h"

typedef struct vector_clock
{
    int *vector;
    int size;
    int id;
} vc;

#define CLOCK_LESS (-2)
#define CLOCK_C_LESS (-1)
#define CLOCK_ERROR 0
#define CLOCK_C_GREATER 1
#define CLOCK_GREATER 2

#define CONCURRENT(x) ((x)==CLOCK_C_LESS || (x)==CLOCK_C_GREATER)

#define CURRENT_PID server.p2p_id
#define l_newVC newVC(server.p2p_count, CURRENT_PID)
#define l_increaseVC(c) increaseVC((c), CURRENT_PID)

vc *newVC(int size, int id);
vc *duplicateVC(const vc *c);
int compareVC(const vc *c1, const vc *c2);
vc *updateVC(vc *tar, const vc *m);
sds VCToSds(const vc *c);
vc *SdsToVC(sds s);
int equalVC(const vc *c1, const vc *c2);
int causally_ready(const vc *current, const vc *next);
inline void deleteVC(vc *c)
{
    zfree(c->vector);
    zfree(c);
}

inline vc *increaseVC(vc *c, int id)
{
    c->id = id;
    c->vector[id]++;
    return c;
}

// get the next vc in sds format, doesn't change the current vc
inline sds nowVC(vc *c)
{
    c->vector[c->id]++;
    sds rtn = VCToSds(c);
    c->vector[c->id]--;
    return rtn;
}

#endif //REDIS_4_0_8_VECTOR_CLOCK_H
