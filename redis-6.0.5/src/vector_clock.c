//
// Created by user on 18-7-3.
//

#include "server.h"
#include "CRDT.h"
#include "vector_clock.h"

int vc_int_buf[100];

vc *_vc_new(int size, int id)
{
    vc *clock = zmalloc(sizeof(vc));
    clock->vector = zmalloc(sizeof(int) * size);
    for (int i = 0; i < size; ++i)
        clock->vector[i] = 0;
    clock->size = size;
    clock->id = id;
    return clock;
}

vc *vc_dup(const vc *c)
{
    vc *clock = zmalloc(sizeof(vc));
    clock->vector = zmalloc(sizeof(int) * c->size);
    for (int i = 0; i < c->size; ++i)
        clock->vector[i] = c->vector[i];
    clock->size = c->size;
    clock->id = c->id;
    return clock;
}

int vc_equal(const vc *c1, const vc *c2)
{
    if (c1 == c2)return 1;
    if (c1->size != c2->size) return 0;
    for (int i = 0; i < c1->size; ++i)
        if (c1->vector[i] != c2->vector[i])
            return 0;
    return 1;
}

int vc_causally_ready(const vc *current, const vc *next)
{
    serverAssert(current->size == next->size);
    int o = next->id;
    for (int i = 0; i < current->size; ++i)
    {
        if (i == o)continue;
        if (next->vector[i] > current->vector[i])return 0;
    }
    if (next->vector[o] == current->vector[o] + 1)
        return 1;
    return 0;
}

int vc_cmp(const vc *c1, const vc *c2)
{
    if (c1->size != c2->size) return VC_CMP_ERR;
    int i = 0;
    for (; i < c1->size && c1->vector[i] == c2->vector[i]; ++i);
    if (i == c1->size)
    {
        if (c1->id == c2->id) return VC_CMP_ERR;
        if (c1->id > c2->id) return VC_C_GREATER;
        if (c1->id < c2->id) return VC_C_LESS;
    }
    if (c1->vector[i] > c2->vector[i])
    {
        for (++i; i < c1->size; ++i)
            if (c1->vector[i] < c2->vector[i])
            {
                if (c1->id > c2->id) return VC_C_GREATER;
                if (c1->id < c2->id) return VC_C_LESS;
            }
        return VC_GREATER;
    }
    if (c1->vector[i] < c2->vector[i])
    {
        for (++i; i < c1->size; ++i)
            if (c1->vector[i] > c2->vector[i])
            {
                if (c1->id > c2->id) return VC_C_GREATER;
                if (c1->id < c2->id) return VC_C_LESS;
            }
        return VC_LESS;
    }
    return VC_CMP_ERR;
}

vc *vc_update(vc *tar, const vc *m)
{
    if (tar->size != m->size)return tar;
    for (int i = 0; i < tar->size; ++i)
        if (m->vector[i] > tar->vector[i])
            tar->vector[i] = m->vector[i];
    return tar;
}

sds vcToSds(const vc *c)
{
    sds p = sdsempty();
    int i;
    for (i = 0; i < c->size - 1; ++i)
        p = sdscatprintf(p, "%d,", c->vector[i]);
    p = sdscatprintf(p, "%d|%d", c->vector[i], c->id);
    return p;
}

vc *sdsToVC(sds s)
{
    int size = 0, id;
    char *p = s;
    while (1)
    {
        vc_int_buf[size] = atoi(p);
        size++;
        while (*p != ',' && *p != '|') p++;
        if (*p == '|')break;
        p++;
    }
    p++;
    id = atoi(p);
    vc *clock = zmalloc(sizeof(vc));
    clock->vector = zmalloc(sizeof(int) * size);
    for (int i = 0; i < size; ++i)
        clock->vector[i] = vc_int_buf[i];
    clock->size = size;
    clock->id = id;
    return clock;
}

/*
void vcnewCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            if (c->argc > 2)
            {
                addReply(c, shared.syntaxerr);
                return;
            }
            robj *vco = lookupKeyWrite(c->db, c->argv[1]);
            if (vco)
            {
                addReply(c, shared.alreadyexisterr);
                return;
            }
            vc *vc = vc_new();
            RARGV_ADD_SDS(vcToSds(vc));
            vc_delete(vc);
        CRDT_EFFECT
            vc *vc = sdsToVC(c->rargv[2]->ptr);
            dbAdd(c->db, c->rargv[1], createObject(OBJ_VECTOR_CLOCK, vc));
            server.dirty += 1;
    CRDT_END
}

void vcgetCommand(client *c)
{
    robj *o;
    if ((o = lookupKeyReadOrReply(c, c->argv[1], shared.nokeyerr)) == NULL)return;
    if (o->type != OBJ_VECTOR_CLOCK)
    {
        addReply(c, shared.wrongtypeerr);
        return;
    }
    addReplyBulkSds(c, vcToSds(o->ptr));
}

void vcincCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            if (c->argc > 2)
            {
                addReply(c, shared.syntaxerr);
                return;
            }
            robj *o;
            if ((o = lookupKeyWriteOrReply(c, c->argv[1], shared.nokeyerr)) == NULL)return;
            if (o->type != OBJ_VECTOR_CLOCK)
            {
                addReply(c, shared.wrongtypeerr);
                return;
            }

            vc *vc = vc_dup(o->ptr);
            vc_inc(vc);
            RARGV_ADD_SDS(vcToSds(vc));
            vc_delete(vc);
        CRDT_EFFECT
            robj *o = lookupKeyWrite(c->db, c->rargv[1]);
            vc *tmp, *vc = sdsToVC(c->rargv[2]->ptr);
            if (vc_cmp(o->ptr, vc) < 0)
            {
                tmp = o->ptr;
                o->ptr = vc;
                vc_delete(tmp);
                server.dirty++;
            }
            else vc_delete(vc);
    CRDT_END
}
*/
