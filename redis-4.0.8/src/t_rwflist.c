//
// Created by user on 19-11-10.
//
#include "server.h"
#include "RWFramework.h"
#include "list_basics.h"


typedef struct rwf_list_element
{
    reh header;
    sds oid;
    leid *pos_id;
    lc *current;
    sds content;

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

#define GET_RLE_NEW(arg_t) \
(rle *) rehHTGet(c->db, c->arg_t[1], NULL, c->arg_t[3], 1, rleNew)

#define GET_RL_HT(arg_t, create) getInnerHT(c->db, c->arg_t[1], NULL, create)

rle *getHead(robj *ht)
{
    sds hname = sdsnew("head");
    robj *value = hashTypeGetValueObject(ht, hname);
    sdsfree(hname);
    if (value == NULL)return NULL;
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

lc *getCurrent(robj *ht)
{
    lc *e;
    sds hname = sdsnew("current");
    robj *value = hashTypeGetValueObject(ht, hname);
    if (value == NULL)
    {
        e = LC_NEW(0);
        RWFHT_SET(ht, hname, lc*, e);
    }
    else
    {
        e = *(lc **) (value->ptr);
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
        int len = *(int *) (value->ptr);
        decrRefCount(value);
        return len;
    }
}

void incrbyLen(robj *ht, int inc)
{
    sds hname = sdsnew("length");
    robj *value = hashTypeGetValueObject(ht, hname);
    int len;
    if (value == NULL)
        len = 0;
    else
    {
        len = *(int *) (value->ptr);
        decrRefCount(value);
    }
    len += inc;
    RWFHT_SET(ht, hname, int, len);
    sdsfree(hname);
}

#define __bold (1<<0)
#define __italic (1<<1)
#define __underline (1<<2)

#define IN_UPDATE_NPR(T, value)\
do{\
    if(e->T##_t==NULL)\
        e->T=value;\
}while(0)

#define IN_UPDATE_PR(T, value)\
do{\
    if(e->T##_t==NULL)\
    {\
        if((value & __##T)==0)\
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
            lc_update(e->current, t);\
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
            lc_update(e->current, t);\
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
    e->oid = NULL;
    e->pos_id = NULL;
    e->current = LC_NEW(0);
    e->content = NULL;

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
static void removeFunc(client *c, rle *e, vc *t)
{
    if (removeCheck((reh *) e, t))
    {
        robj *ht = GET_RL_HT(rargv, 0);
        if (EXISTS(e)) incrbyLen(ht, -1);
        REH_RMV_FUNC(e, t);
        RMV_LC(e->font_t);
        RMV_LC(e->size_t);
        RMV_LC(e->color_t);
        RMV_LC(e->bold_t);
        RMV_LC(e->italic_t);
        RMV_LC(e->underline_t);
        e->current->x = 0;
        server.dirty++;
    }
}

void rwflinsertCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_HASH, 9);
            for (int i = 5; i < 9; ++i)
                CHECK_ARG_TYPE_INT(c->argv[i]);
            rle *pre = GET_RLE(argv, 0);
            if (pre == NULL)
            {
                sdstolower(c->argv[2]->ptr);
                if (strcmp(c->argv[2]->ptr, "null") != 0)
                {
                    sds errs = sdscatfmt(sdsempty(), "-No pre element %S in the list.\r\n", c->argv[2]->ptr);
                    addReplySds(c, errs);
                    return;
                }
            }
            rle *e = GET_RLE_NEW(argv);
            PREPARE_PRECOND_ADD(e);
            if (e->oid == NULL)
            {
                leid *left = pre == NULL ? NULL : pre->pos_id;
                leid *right;
                if (pre == NULL)
                {
                    rle *head = getHead(GET_RL_HT(argv, 1));
                    right = head == NULL ? NULL : head->pos_id;
                }
                else
                    right = pre->next == NULL ? NULL : pre->next->pos_id;
                leid *id = constructLeid(left, right, getCurrent(GET_RL_HT(argv, 1)));
                RARGV_ADD_SDS(leidToSds(id));
                leidFree(id);
            }
            else
                RARGV_ADD_SDS(leidToSds(e->pos_id));
            ADD_CR_NON_RMV(e);
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            rle *e = GET_RLE_NEW(rargv);
            removeFunc(c, e, t);
            if (insertCheck((reh *) e, t))
            {
                robj *ht = GET_RL_HT(rargv, 1);
                if (!EXISTS(e)) incrbyLen(ht, 1);
                PID(e) = t->id;
                // The element is newly inserted.
                if (e->oid == NULL)
                {
                    e->oid = sdsdup(c->rargv[3]->ptr);
                    e->content = sdsdup(c->rargv[4]->ptr);
                    e->pos_id = sdsToLeid(c->rargv[9]->ptr);
                    rle *head = getHead(ht);
                    if (head == NULL)
                    {
                        setHead(ht, e);
                    }
                    else if (leid_cmp(e->pos_id, head->pos_id) < 0)
                    {
                        setHead(ht, e);
                        e->next = head;
                        head->prev = e;
                    }
                    else
                    {
                        rle *pre = GET_RLE(rargv, 0);
                        rle *p, *q;
                        if (pre != NULL)
                            p = pre;
                        else p = head;
                        q = p->next;
                        while (q != NULL && leid_cmp(e->pos_id, q->pos_id) > 0)
                        {
                            p = q;
                            q = q->next;
                        }
                        p->next = e;
                        e->prev = p;
                        e->next = q;
                        if (q != NULL)
                            q->prev = e;
                    }
                }
                long long tmp;
                getLongLongFromObject(c->rargv[5], &tmp);
                IN_UPDATE_NPR(font, tmp);
                getLongLongFromObject(c->rargv[6], &tmp);
                IN_UPDATE_NPR(size, tmp);
                getLongLongFromObject(c->rargv[7], &tmp);
                IN_UPDATE_NPR(color, tmp);
                getLongLongFromObject(c->rargv[8], &tmp);
                IN_UPDATE_PR(bold, tmp);
                IN_UPDATE_PR(italic, tmp);
                IN_UPDATE_PR(underline, tmp);
                server.dirty++;
            }
            deleteVC(t);
    CRDT_END
}

void rwflupdateCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_HASH, 5);
            CHECK_ARG_TYPE_INT(c->argv[4]);
            rle *e = GET_RLE(argv, 0);
            PREPARE_PRECOND_NON_ADD(e);
            e->current->x++;
            RARGV_ADD_SDS(lcToSds(e->current));
            e->current->x--;
            ADD_CR_NON_RMV(e);
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            rle *e = GET_RLE(rargv, 1);
            lc *lt = sdsToLc(c->rargv[5]->ptr);
            long long value;
            getLongLongFromObject(c->rargv[4], &value);
            removeFunc(c, e, t);
            if (updateCheck((reh *) e, t))
            {
                acquired_update(e, c->rargv[3]->ptr, lt, value);
                server.dirty++;
            }
            deleteVC(t);
            zfree(lt);
    CRDT_END
}

void rwflremCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_HASH, 3);
            rle *e = GET_RLE(argv, 0);
            PREPARE_PRECOND_NON_ADD(e);
            ADD_CR_RMV(e);
        CRDT_EFFECT
            rle *e = GET_RLE(rargv, 1);
            vc *t = CR_GET_LAST;
            removeFunc(c, e, t);
            deleteVC(t);
    CRDT_END
}

void rwfllenCommand(client *c)
{
    robj *o = lookupKeyReadOrReply(c, c->argv[1], shared.czero);
    if (o == NULL || checkType(c, o, OBJ_HASH)) return;
    addReplyLongLong(c, getLen(o));
}

void rwfllistCommand(client *c)
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
