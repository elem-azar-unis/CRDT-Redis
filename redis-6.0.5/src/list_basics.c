//
// Created by admin on 2020/5/7.
//

#include "list_basics.h"

static position leid_buf[1024];

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
        leid_buf[size].pos = (unsigned int)atoi(p);
        while (*p != ',')
            p++;
        p++;
        leid_buf[size].pid = atoi(p);
        while (*p != ',')
            p++;
        p++;
        leid_buf[size].count = atoi(p);
        size++;
        while (*p != '|' && *p != '\0')
            p++;
        if (*p == '\0') break;
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
        if (pos_cmp(&id1->p[i], &id2->p[i]) != 0) break;
    if (i == n) return id1->num - id2->num;
    return pos_cmp(&id1->p[i], &id2->p[i]);
}

leid *constructLeid(leid *p, leid *q, vc *t)
{
    int index = 0, count = 0;
    for (int i = 0; i < t->size; ++i)
        count += t->vector[i];
    while (rprefix(q, index) - lprefix(p, index) < 2)
        index++;
    unsigned int left = lprefix(p, index);
    unsigned int right = rprefix(q, index);
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
        for (int i = p->num; i < index; i++)
        {
            rtn->p[i].pos = (q != NULL && i < q->num) ? q->p[i].pos : 0;
            rtn->p[i].pid = (q != NULL && i < q->num) ? q->p[i].pid : t->id;
            rtn->p[i].count = (q != NULL && i < q->num) ? q->p[i].count : count;
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
    unsigned int step = right - left - 2;
    if (step != 0) step = step < RDM_STEP ? rand() % step : rand() % RDM_STEP;
    rtn->p[index].pos = left + step + 1;
    rtn->p[index].pid = t->id;
    rtn->p[index].count = count;
    return rtn;
}

void *getHead(robj *ht)
{
    sds hname = sdsnew("head");
    robj *value = hashTypeGetValueObject(ht, hname);
    sdsfree(hname);
    if (value == NULL) return NULL;
    void *e = *(void **)(value->ptr);
    decrRefCount(value);
    return e;
}

void setHead(robj *ht, void *e)
{
    sds hname = sdsnew("head");
    RWFHT_SET(ht, hname, void *, e);
    sdsfree(hname);
}

vc *getCurrent(robj *ht)
{
    vc *e;
    sds hname = sdsnew("current");
    robj *value = hashTypeGetValueObject(ht, hname);
    if (value == NULL)
    {
        e = vc_new();
        RWFHT_SET(ht, hname, vc *, e);
    }
    else
    {
        e = *(vc **)(value->ptr);
        decrRefCount(value);
    }
    sdsfree(hname);
    return e;
}

int getLen(robj *ht)
{
    sds hname = sdsnew("length");
    robj *value = hashTypeGetValueObject(ht, hname);
    sdsfree(hname);
    if (value == NULL)
        return 0;
    else
    {
        int len = *(int *)(value->ptr);
        decrRefCount(value);
        return len;
    }
}

void incrLen(robj *ht, int inc)
{
    sds hname = sdsnew("length");
    robj *value = hashTypeGetValueObject(ht, hname);
    int len;
    if (value == NULL)
        len = 0;
    else
    {
        len = *(int *)(value->ptr);
        decrRefCount(value);
    }
    len += inc;
    RWFHT_SET(ht, hname, int, len);
    sdsfree(hname);
}
