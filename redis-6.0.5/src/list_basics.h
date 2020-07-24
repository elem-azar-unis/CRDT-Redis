//
// Created by admin on 2020/5/7.
//

#ifndef REDIS_LIST_BASICS_H
#define REDIS_LIST_BASICS_H

#include "CRDT.h"
#include "RWFramework.h"
#include "server.h"

#define FORALL_NORMAL(ACTION) \
    ACTION(font)              \
    ACTION(size)              \
    ACTION(color)

#define FORALL_BITMAP(ACTION) \
    ACTION(bold)              \
    ACTION(italic)            \
    ACTION(underline)

#define FORALL(ACTION)    \
    FORALL_NORMAL(ACTION) \
    FORALL_BITMAP(ACTION)

#define LIST_PR_NUM 6
#define LIST_PR_NORMAL_NUM 3

enum property_type
{
    __bold = 1 << 0,
    __italic = 1 << 1,
    __underline = 1 << 2
};

#define BASE (1 << 24)
#define RDM_STEP 128
typedef struct position
{
    int pos;
    int pid;
    int count;
} position;

static inline int pos_cmp(const position *p1, const position *p2)
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

static inline void leidFree(leid *id)
{
    zfree(id->p);
    zfree(id);
}

sds leidToSds(const leid *p);

leid *sdsToLeid(sds s);

int leid_cmp(const leid *id1, const leid *id2);

static inline int lprefix(leid *p, int i)
{
    if (p == NULL) return 0;
    if (i >= p->num) return 0;
    return p->p[i].pos;
}

static inline int rprefix(leid *p, int i)
{
    if (p == NULL) return BASE;
    if (i >= p->num) return BASE;
    return p->p[i].pos;
}

leid *constructLeid(leid *p, leid *q, vc *t);

#define GET_LIST_HT(arg_t, create) getInnerHT(c->db, c->arg_t[1], NULL, create)

void *getHead(robj *ht);

void setHead(robj *ht, void *e);

vc *getCurrent(robj *ht);

int getLen(robj *ht);

void incrLen(robj *ht, int inc);

#ifdef CRDT_OVERHEAD

#define LEID_SIZE(id) (id == NULL ? 0 : (sizeof(leid) + id->num * sizeof(position)))

#endif

#endif  // REDIS_LIST_BASICS_H
