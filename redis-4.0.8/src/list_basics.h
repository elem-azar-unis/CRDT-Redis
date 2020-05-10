//
// Created by admin on 2020/5/7.
//

#ifndef REDIS_4_0_8_LIST_BASICS_H
#define REDIS_4_0_8_LIST_BASICS_H

#include "RWFramework.h"
#include "server.h"

#define FORALL_NPR(ACTION)   \
    ACTION(font)             \
    ACTION(size)             \
    ACTION(color)

#define FORALL_PR(ACTION)   \
    ACTION(bold)            \
    ACTION(italic)          \
    ACTION(underline)

#define FORALL(ACTION) FORALL_NPR(ACTION) FORALL_PR(ACTION)


enum property_type
{
    __bold = 1 << 0,
    __italic = 1 << 1,
    __underline = 1 << 2
};

#define BASE (1<<24)
#define RDM_STEP 8
typedef struct position
{
    unsigned int pos;
    int pid;
    int count;
} position;

inline int pos_cmp(const position *p1, const position *p2)
{
    if (p1->pos != p2->pos) return p1->pos - p2->pos;
    if (p1->pid != p2->pid) return p1->pid - p2->pid;
    return p1->count - p2->count;
}

typedef struct list_element_identifier
{
    int num;
    position *p;
} leid;

inline void leidFree(leid *id)
{
    zfree(id->p);
    zfree(id);
}

sds leidToSds(const leid *p);

leid *sdsToLeid(sds s);

int leid_cmp(const leid *id1, const leid *id2);

inline int lprefix(leid *p, int i)
{
    if (p == NULL)return 0;
    if (i >= p->num)return 0;
    return p->p[i].pos;
}

inline int rprefix(leid *p, int i)
{
    if (p == NULL)return BASE;
    if (i >= p->num)return BASE;
    return p->p[i].pos;
}

leid *constructLeid(leid *p, leid *q, vc *t);

#define GET_RFWL_HT(arg_t, create) getInnerHT(c->db, c->arg_t[1], NULL, create)

void *getHead(robj *ht);

void setHead(robj *ht, void *e);

vc *getCurrent(robj *ht);

int getLen(robj *ht);

void incrbyLen(robj *ht, int inc);

#endif //REDIS_4_0_8_LIST_BASICS_H
