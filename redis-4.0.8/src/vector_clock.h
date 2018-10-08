//
// Created by user on 18-6-21.
//

#ifndef REDIS_4_0_8_VECTOR_CLOCK_H

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

#define PID server.p2p_id
#define l_newVC newVC(server.p2p_count, PID)
#define l_increaseVC(c) increaseVC((c), PID)

vc *newVC(int size, int id);
void deleteVC(vc *c);
vc *increaseVC(vc *c, int id);
vc *duplicateVC(const vc *c);
int compareVC(const vc *c1, const vc *c2);
vc *updateVC(vc *tar, const vc *m);
sds VCToSds(const vc *c);
vc *SdsToVC(sds s);
int equalVC(const vc *c1, const vc *c2);
int causally_ready(const vc *current, const vc *next);

#define REDIS_4_0_8_VECTOR_CLOCK_H

#endif //REDIS_4_0_8_VECTOR_CLOCK_H
