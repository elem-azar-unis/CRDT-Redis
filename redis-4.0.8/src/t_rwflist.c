//
// Created by user on 19-11-10.
//
#include "server.h"
#include "RWFramework.h"

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

position leid_buf[100];

sds leidToSds(const leid *p)
{
    sds s = sdsempty();
    int i;
    for (i = 0; i < p->num - 1; ++i)
    {
        s = sdscatprintf(s, "%d,%d,%d|", p->p[i].pos, p->p[i].pid, p->p[i].count);
    }
    s = sdscatprintf(s, "%d,%d,%d", p->p[i].pos, p->p[i].pid, p->p[i].count);
    return s;
}

leid *sdsToLeid(sds s)
{
    int size = 0;
    char *p = s;
    while (1)
    {
        leid_buf[size].pos = (unsigned int) atoi(p);
        while (*p != ',') p++;
        p++;
        leid_buf[size].pid = atoi(p);
        while (*p != ',') p++;
        p++;
        leid_buf[size].count = atoi(p);
        size++;
        while (*p != '|' && *p != '\0') p++;
        if (*p == '\0')break;
        p++;
    }
    leid *rtn = zmalloc(sizeof(leid));
    rtn->num = size;
    rtn->p = zmalloc(size * sizeof(position));
    for (int i = 0; i < size; ++i)
    {
        rtn->p[i].pos = leid_buf[i].pos;
        rtn->p[i].pid = leid_buf[i].pid;
        rtn->p[i].count = leid_buf[i].count;
    }
    return rtn;
}

int leid_cmp(const leid *id1, const leid *id2)
{
    int n = id1->num < id2->num ? id1->num : id2->num;
    int i = 0;
    for (; i < n; i++)
        if (pos_cmp(&id1->p[i], &id2->p[i]) != 0)break;
    if (i == n)return id1->num - id2->num;
    return pos_cmp(&id1->p[i], &id2->p[i]);
}

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

leid *constructLeid(leid *p, leid *q, lc *t)
{
    int index = 0;
    while (rprefix(q, index) - lprefix(p, index) < 2)index++;
    int left = lprefix(p, index);
    int right = rprefix(q, index);
    // left<new<right
    leid *rtn = zmalloc(sizeof(leid));
    rtn->num = index + 1;
    rtn->p = zmalloc(sizeof(position) * (index + 1));
    if (p != NULL)
    {
        for (int i = 0; i < index && i < p->num; i++)
        {
            rtn->p[i].pos = p->p[i].pos;
            rtn->p[i].pid = p->p[i].pid;
            rtn->p[i].count = p->p[i].count;
        }
        for (int i = p->num; i < index && i < p->num; i++)
        {
            rtn->p[i].pos = 0;
            rtn->p[i].pid = t->id;
            rtn->p[i].count = t->x;
        }
    }
    else
    {
        for (int i = 0; i < index; i++)
        {
            rtn->p[i].pos = 0;
            rtn->p[i].pid = 0;
            rtn->p[i].count = 0;
        }
    }
    int step = right - left - 2;
    step = step < RDM_STEP ? step : RDM_STEP;
    rtn->p[index].pos = left + (unsigned int) (rand() % step) + 1;
    rtn->p[index].pid = t->id;
    rtn->p[index].count = t->x;
    t->x++;
    return rtn;
}

typedef struct rwf_list_element
{
    reh header;
    sds oid;
    leid *pos_id;
    lc *current;
    sds content;

    // flags indicate whether these properties are innate or have been applied with some acquired value.
    int state_mask;
    lc *font_t, *size_t, *color_t, *bold_t, *italic_t, *underline_t;
    int font;
    int size;
    int color;
    int property;

    struct rwf_list_element *prev;
    struct rwf_list_element *next;
} rle;

#define GET_RLE(arg_t, create) \
(rle *) rehHTGet(c->db, c->arg_t[1], NULL, c->arg_t[2], create, rleNew)

#define GET_RL_HT(arg_t, create) getInnerHT(c->db, c->arg_t[1], NULL, create)

rle *getHead(robj *ht)
{
    sds hname = sdsnew("head");
    robj *value = hashTypeGetValueObject(ht, hname);
    sdsfree(hname);
    rle *e = *(rle **) (value->ptr);
    decrRefCount(value);
    return e;
}

void setHead(robj *ht, rle *e)
{
    sds hname = sdsnew("head");
    RWFHT_SET(ht, hname, rle*, e);
    sdsfree(hname);
}

int getLen(robj *ht)
{
    sds hname = sdsnew("head");
    robj *value = hashTypeGetValueObject(ht, hname);
    sdsfree(hname);
    int len = *(int *) (value->ptr);
    decrRefCount(value);
    return len;
}

void incrbyLen(robj *ht, int inc)
{
    sds hname = sdsnew("head");
    robj *value = hashTypeGetValueObject(ht, hname);
    int len = *(int *) (value->ptr);
    decrRefCount(value);
    len += inc;
    RWFHT_SET(ht, hname, int, len);
    sdsfree(hname);
}

// the flag masks for state_mask
#define font_m (1<<0)
#define size_m (1<<1)
#define color_m (1<<2)
#define bold_m (1<<3)
#define italic_m (1<<4)
#define underline_m (1<<5)

#define __bold (1<<0)
#define __italic (1<<1)
#define __underline (1<<2)

#define IN_UPDATE_NPR(T, value)\
do{\
    if(!(e->state_mask & T##_m))\
        e->T=value;\
}while(0)

#define IN_UPDATE_PR(T, value)\
do{\
    if(!(e->state_mask & T##_m))\
    {\
        if((value)==0)\
            e->property &=~ __##T;\
        else\
            e->property |= __##T;\
    }\
}while(0)

#define AC_UPDATE_NPR(T)\
do{\
    if(strcmp(type,#T)==0)\
    {\
        if((e->T##_t)==NULL || lc_cmp(e->T##_t,t)<=0)\
        {\
            LC_COPY(e->T##_t,t);\
            e->T=value;\
            e->state_mask |= T##_m;\
        }\
        return;\
    }\
}while(0)

#define AC_UPDATE_PR(T)\
do{\
    if(strcmp(type,#T)==0)\
    {\
        if((e->T##_t)==NULL || lc_cmp(e->T##_t,t)<=0)\
        {\
            LC_COPY(e->T##_t,t);\
            if(value==0)\
                e->property &=~ __##T;\
            else\
                e->property |= __##T;\
            e->state_mask |= T##_m;\
        }\
        return;\
    }\
}while(0)

void acquired_update(rle *e, sds type, lc *t, int value)
{
    sdstolower(type);
    AC_UPDATE_NPR(font);
    AC_UPDATE_NPR(size);
    AC_UPDATE_NPR(color);
    AC_UPDATE_PR(bold);
    AC_UPDATE_PR(italic);
    AC_UPDATE_PR(underline);
}

reh *rleNew()
{
    rle *e = zmalloc(sizeof(rle));
    REH_INIT(e);
    e->pos_id = NULL;
    e->current = LC_NEW(0);
    e->content = NULL;
    e->state_mask = 0;

    e->font_t = NULL;
    e->size_t = NULL;
    e->color_t = NULL;
    e->bold_t = NULL;
    e->italic_t = NULL;
    e->underline_t = NULL;

    e->prev = NULL;
    e->next = NULL;
    return (reh *) e;
}

#define RMV_LC(t)\
do{\
    if((t)!=NULL)\
    {\
        zfree(t);\
        (t)=NULL;\
    }\
}while(0)

// This doesn't free t.
void removeFunc(client *c, rle *e, vc *t)
{
    if (removeCheck((reh *) e, t))
    {
        REH_RMV_FUNC(e, t);
        e->state_mask = 0;
        RMV_LC(e->font_t);
        RMV_LC(e->size_t);
        RMV_LC(e->color_t);
        RMV_LC(e->bold_t);
        RMV_LC(e->italic_t);
        RMV_LC(e->underline_t);
        incrbyLen(GET_RL_HT(rargv, 0), -1);
        server.dirty++;
    }
}

void rlinsertCommand(client *c);

void rlupdateCommand(client *c);

void rlremCommand(client *c);

void rllenCommand(client *c)
{
    robj *o = lookupKeyReadOrReply(c, c->argv[1], shared.czero);
    if (o == NULL || checkType(c, o, OBJ_HASH)) return;
    addReplyLongLong(c, getLen(o));
}

void rllistCommand(client *c)
{
    robj *o = lookupKeyReadOrReply(c, c->argv[1], shared.emptymultibulk);
    if (o == NULL || checkType(c, o, OBJ_HASH)) return;
    addReplyMultiBulkLen(c, getLen(o));
    rle *e = getHead(o);
    while (e != NULL)
    {
        if (EXISTS(e))
        {
            addReplyMultiBulkLen(c, 6);
            addReplyBulkCBuffer(c, e->oid, sdslen(e->oid));
            addReplyBulkCBuffer(c, e->content, sdslen(e->content));
            addReplyBulkLongLong(c, e->font);
            addReplyBulkLongLong(c, e->size);
            addReplyBulkLongLong(c, e->color);
            addReplyBulkLongLong(c, e->property);
        }
        e = e->next;
    }
}
