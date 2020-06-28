//
// Created by user on 19-11-10.
//
#include "server.h"
#include "CRDT.h"
#include "RWFramework.h"
#include "list_basics.h"
#include "lamport_clock.h"

#define DEFINE_NPR(p) int p;
#define DEFINE_A(p) lc *p##_t;
typedef struct rwf_list_element
{
    reh header;
    sds oid;
    leid *pos_id;
    lc *current;
    sds content;

    FORALL(DEFINE_A)
    FORALL_NPR(DEFINE_NPR)
    int property;

    struct rwf_list_element *prev;
    struct rwf_list_element *next;
} rwfle;
#undef DEFINE_NPR
#undef DEFINE_A

#define GET_RWFLE(arg_t, create) \
    (rwfle *)rehHTGet(c->db, c->arg_t[1], NULL, c->arg_t[2], create, (rehNew_func_t)rwfleNew)

#define GET_RWFLE_NEW(arg_t) \
    (rwfle *)rehHTGet(c->db, c->arg_t[1], NULL, c->arg_t[3], 1, (rehNew_func_t)rwfleNew)

void acquired_update(rwfle *e, sds type, lc *t, int value)
{
    sdstolower(type);
#define AC_UPDATE_NPR(T)\
    if(strcmp(type,#T)==0)\
    {\
        if((e->T##_t)==NULL || lc_cmp(e->T##_t,t)<=0)\
        {\
            lc_copy(e->T##_t,t);\
            e->T=value;\
            lc_update(e->current, t);\
        }\
        return;\
    }
    FORALL_NPR(AC_UPDATE_NPR);
#undef AC_UPDATE_NPR
#define AC_UPDATE_PR(T)\
    if(strcmp(type,#T)==0)\
    {\
        if((e->T##_t)==NULL || lc_cmp(e->T##_t,t)<=0)\
        {\
            lc_copy(e->T##_t,t);\
            if(value==0)\
                e->property &=~ __##T;\
            else\
                e->property |= __##T;\
            lc_update(e->current, t);\
        }\
        return;\
    }
    FORALL_PR(AC_UPDATE_PR);
#undef AC_UPDATE_PR
}

rwfle *rwfleNew()
{
    rwfle *e = zmalloc(sizeof(rwfle));
    REH_INIT(e);
    e->oid = NULL;
    e->pos_id = NULL;
    e->current = lc_new(0);
    e->content = NULL;

#define TMP_ACTION(p) e->p##_t = NULL;
    FORALL(TMP_ACTION);
#undef TMP_ACTION

    e->prev = NULL;
    e->next = NULL;
    return e;
}


// This doesn't free t.
static void removeFunc(client *c, rwfle *e, vc *t)
{
    if (removeCheck((reh *) e, t))
    {
        robj *ht = GET_LIST_HT(rargv, 0);
        if (EXISTS(e)) incrbyLen(ht, -1);
        REH_RMV_FUNC(e, t);
#define RMV_LC(p)            \
    if ((e->p##_t) != NULL)  \
    {                        \
        lc_delete(e->p##_t); \
        (e->p##_t) = NULL;   \
    }
        FORALL(RMV_LC);
#undef RMV_LC
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
            rwfle *pre = GET_RWFLE(argv, 0);
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
            rwfle *e = GET_RWFLE_NEW(argv);
            PREPARE_PRECOND_ADD(e);
            if (e->oid == NULL)
            {
                leid *left = pre == NULL ? NULL : pre->pos_id;
                leid *right;
                if (pre == NULL)
                {
                    rwfle *head = getHead(GET_LIST_HT(argv, 1));
                    right = head == NULL ? NULL : head->pos_id;
                }
                else
                    right = pre->next == NULL ? NULL : pre->next->pos_id;
                vc *cur = getCurrent(GET_LIST_HT(argv, 1));
                leid *id = constructLeid(left, right, cur);
                vc_inc(cur);
                RARGV_ADD_SDS(leidToSds(id));
                leidFree(id);
            }
            else
                RARGV_ADD_SDS(leidToSds(e->pos_id));
            ADD_CR_NON_RMV(e);
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            rwfle *e = GET_RWFLE_NEW(rargv);
            removeFunc(c, e, t);
            if (insertCheck((reh *) e, t))
            {
                robj *ht = GET_LIST_HT(rargv, 1);
                if (!EXISTS(e)) incrbyLen(ht, 1);
                PID(e) = t->id;
                // The element is newly inserted.
                if (e->oid == NULL)
                {
                    e->oid = sdsdup(c->rargv[3]->ptr);
                    e->content = sdsdup(c->rargv[4]->ptr);
                    e->pos_id = sdsToLeid(c->rargv[9]->ptr);
                    rwfle *head = getHead(ht);
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
                        rwfle *pre = GET_RWFLE(rargv, 0);
                        rwfle *p, *q;
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


#define IN_UPDATE_NPR(T)\
    if(e->T##_t==NULL) e->T=T;

#define IN_UPDATE_PR(T)\
    if(e->T##_t==NULL)\
    {\
        if((property & __##T)==0)\
            e->property &=~ __##T;\
        else\
            e->property |= __##T;\
    }
                long long font, size, color, property;
                getLongLongFromObject(c->rargv[5], &font);
                getLongLongFromObject(c->rargv[6], &size);
                getLongLongFromObject(c->rargv[7], &color);
                FORALL_NPR(IN_UPDATE_NPR);
                getLongLongFromObject(c->rargv[8], &property);
                FORALL_PR(IN_UPDATE_PR);
#undef IN_UPDATE_NPR
#undef IN_UPDATE_PR
                server.dirty++;
            }
            vc_delete(t);
    CRDT_END
}

void rwflupdateCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_HASH, 5);
            CHECK_ARG_TYPE_INT(c->argv[4]);
            rwfle *e = GET_RWFLE(argv, 0);
            PREPARE_PRECOND_NON_ADD(e);
            e->current->x++;
            RARGV_ADD_SDS(lcToSds(e->current));
            e->current->x--;
            ADD_CR_NON_RMV(e);
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            rwfle *e = GET_RWFLE(rargv, 1);
            lc *lt = sdsToLc(c->rargv[5]->ptr);
            long long value;
            getLongLongFromObject(c->rargv[4], &value);
            removeFunc(c, e, t);
            if (updateCheck((reh *) e, t))
            {
                acquired_update(e, c->rargv[3]->ptr, lt, value);
                server.dirty++;
            }
            vc_delete(t);
            lc_delete(lt);
    CRDT_END
}

void rwflremCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_HASH, 3);
            rwfle *e = GET_RWFLE(argv, 0);
            PREPARE_PRECOND_NON_ADD(e);
            ADD_CR_RMV(e);
        CRDT_EFFECT
            rwfle *e = GET_RWFLE(rargv, 1);
            vc *t = CR_GET_LAST;
            removeFunc(c, e, t);
            vc_delete(t);
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
    robj *o = lookupKeyReadOrReply(c, c->argv[1], shared.emptyarray);
    if (o == NULL || checkType(c, o, OBJ_HASH)) return;
    addReplyArrayLen(c, getLen(o));
    rwfle *e = getHead(o);
    while (e != NULL)
    {
        if (EXISTS(e))
        {
            addReplyArrayLen(c, 6);
            addReplyBulkCBuffer(c, e->oid, sdslen(e->oid));
            addReplyBulkCBuffer(c, e->content, sdslen(e->content));
#define TMP_ACTION(p) addReplyBulkLongLong(c, e->p);
            FORALL_NPR(TMP_ACTION);
#undef TMP_ACTION
            addReplyBulkLongLong(c, e->property);
        }
        e = e->next;
    }
}

#ifdef CRDT_OPCOUNT
void rwflopcountCommand(client *c)
{
    addReplyLongLong(c, op_count_get());
}
#endif

#ifdef CRDT_OVERHEAD
void rwfloverheadCommand(client *c)
{
    addReplyLongLong(c, ovhd_get());
}
#endif
