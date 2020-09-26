//
// Created by user on 20-04-12.
//
#include "RWFramework.h"
#include "list_basics.h"
#include "server.h"
#ifdef CRDT_OVERHEAD

#define RLE_SIZE (sizeof(rle) + 2 * sizeof(list))
#define RLE_ASE_SIZE (sizeof(rl_ase) + sizeof(listNode) + (1 + LIST_PR_NUM) * VC_SIZE)
#define RLE_RSE_SIZE (VC_SIZE + sizeof(listNode))
#define RLE_OPS_SIZE (sizeof(rl_cmd) + sizeof(listNode) + VC_SIZE)

#define RLE_SIZE_ADDITIONAL(e) (sdslen(e->oid) + sdslen(e->content) + LEID_SIZE(e->pos_id))
#define RLE_SIZE_ESSENTIAL(e)                                              \
    (2 * sizeof(sds) + sdslen(e->oid) + sdslen(e->content) + sizeof(rle *) \
     + (1 + LIST_PR_NORMAL_NUM) * sizeof(int))

#endif

#define DEFINE_NORMAL(p) int p;
#define PROPERTY_VC(p) vc *p##_t;
typedef struct rlist_aset_element
{
    vc *t;
    FORALL(PROPERTY_VC)
    FORALL_NORMAL(DEFINE_NORMAL)
    int property;
} rl_ase;
#undef DEFINE_NORMAL
#undef PROPERTY_VC

rl_ase *rl_aseNew(vc *t)
{
    rl_ase *a = zmalloc(sizeof(rl_ase));
    a->t = vc_dup(t);
#define TMP_ACTION(p) a->p##_t = vc_dup(t);
    FORALL(TMP_ACTION);
#undef TMP_ACTION
#ifdef CRDT_OVERHEAD
    ovhd_inc(RLE_ASE_SIZE);
#endif
    return a;
}

void rl_aseDelete(rl_ase *a)
{
    vc_delete(a->t);
#define TMP_ACTION(p) vc_delete(a->p##_t);
    FORALL(TMP_ACTION);
#undef TMP_ACTION
    zfree(a);
#ifdef CRDT_OVERHEAD
    ovhd_inc(-RLE_ASE_SIZE);
#endif
}

enum rl_cmd_type
{
    RLADD,
    RLUPDATE,
    RLREM
};
typedef struct rlist_unready_command
{
    enum rl_cmd_type type;
    int argc;
    robj **argv;
    vc *t;
} rl_cmd;

// get the ownership of t
rl_cmd *rl_cmdNew(enum rl_cmd_type type, client *c, vc *t)
{
    rl_cmd *cmd = zmalloc(sizeof(rl_cmd));
    cmd->type = type;
    cmd->t = t;
    cmd->argc = c->rargc - 2;
    cmd->argv = zmalloc(sizeof(robj *) * cmd->argc);
#ifdef CRDT_OVERHEAD
    ovhd_inc(RLE_OPS_SIZE + (sizeof(robj *) + sizeof(robj)) * cmd->argc);
#endif
    for (int i = 0; i < cmd->argc; ++i)
    {
        cmd->argv[i] = c->rargv[i + 1];
        incrRefCount(cmd->argv[i]);
#ifdef CRDT_OVERHEAD
        ovhd_inc(sdslen((sds)(cmd->argv[i]->ptr)));
#endif
    }
    return cmd;
}

void rl_cmdDelete(rl_cmd *cmd)
{
#ifdef CRDT_OVERHEAD
    ovhd_inc(-(RLE_OPS_SIZE + (sizeof(robj *) + sizeof(robj)) * cmd->argc));
#endif
    vc_delete(cmd->t);
    for (int i = 0; i < cmd->argc; ++i)
    {
#ifdef CRDT_OVERHEAD
        ovhd_inc(-sdslen((sds)(cmd->argv[i]->ptr)));
#endif
        decrRefCount(cmd->argv[i]);
    }
    zfree(cmd->argv);
    zfree(cmd);
}

list *getOps(robj *ht)
{
    list *e;
    sds hname = sdsnew("ops");
    robj *value = hashTypeGetValueObject(ht, hname);
    if (value == NULL)
    {
        e = listCreate();
        RWFHT_SET(ht, hname, list *, e);
#ifdef CRDT_OVERHEAD
        ovhd_inc(sizeof(list));
#endif
    }
    else
    {
        e = *(list **)(value->ptr);
        decrRefCount(value);
    }
    sdsfree(hname);
    return e;
}

#define LOOKUP(e) (listLength((e)->aset) != 0 && listLength((e)->rset) == 0)
typedef struct rw_list_element
{
    sds oid;
    leid *pos_id;
    sds content;

    rl_ase *value;
    list *aset;
    list *rset;

    struct rw_list_element *next;
} rle;

rle *rleNew()
{
#ifdef CRDT_OVERHEAD
    ovhd_inc(RLE_SIZE);
#endif
    rle *e = zmalloc(sizeof(rle));
    e->oid = NULL;
    e->pos_id = NULL;
    e->content = NULL;

    e->value = NULL;
    e->aset = listCreate();
    e->rset = listCreate();

    e->next = NULL;
    return e;
}

#define GET_RLE(db, tname, key, create) \
    (rle *)rehHTGet(db, tname, NULL, key, create, (rehNew_func_t)rleNew)

// doesn't free t, doesn't own t
static void insertFunc(redisDb *db, robj *ht, robj **argv, vc *t)
{
    vc_update(getCurrent(ht), t);
    server.dirty++;

    rle *e = GET_RLE(db, argv[0], argv[2], 1);
    int pre_insert = LOOKUP(e);
    // The element is newly inserted.
    if (e->oid == NULL)
    {
        e->oid = sdsdup(argv[2]->ptr);
        e->content = sdsdup(argv[3]->ptr);
        e->pos_id = sdsToLeid(argv[8]->ptr);
#ifdef CRDT_OVERHEAD
        ovhd_inc(RLE_SIZE_ADDITIONAL(e));
#endif
        rle *head = getHead(ht);
        if (head == NULL) { setHead(ht, e); }
        else if (leid_cmp(e->pos_id, head->pos_id) < 0)
        {
            setHead(ht, e);
            e->next = head;
        }
        else
        {
            rle *pre = GET_RLE(db, argv[0], argv[1], 0);
            rle *p, *q;
            if (pre != NULL && pre->pos_id != NULL)
                p = pre;
            else
                p = head;
            q = p->next;
            while (q != NULL && leid_cmp(e->pos_id, q->pos_id) > 0)
            {
                p = q;
                q = q->next;
            }
            p->next = e;
            e->next = q;
        }
    }

    rl_ase *a = rl_aseNew(t);
#define TMP_ACTION(T) a->T = T;
    long long font, size, color, property;
    getLongLongFromObject(argv[4], &font);
    getLongLongFromObject(argv[5], &size);
    getLongLongFromObject(argv[6], &color);
    FORALL_NORMAL(TMP_ACTION);
    getLongLongFromObject(argv[7], &property);
    a->property = property;
#undef TMP_ACTION
    listAddNodeTail(e->aset, a);

    listNode *ln;
    listIter li;
    listRewind(e->rset, &li);
    while ((ln = listNext(&li)))
    {
        vc *r = ln->value;
        if (vc_cmp(r, t) == VC_LESS)
        {
            listDelNode(e->rset, ln);
            vc_delete(r);
#ifdef CRDT_OVERHEAD
            ovhd_inc(-RLE_RSE_SIZE);
#endif
        }
    }
    if (e->value == NULL || e->value->t->id < t->id) e->value = a;

    if (pre_insert == 0 && LOOKUP(e))
    {
        incrLen(ht, 1);
#ifdef CRDT_OVERHEAD
        ovhd_inc(-RLE_SIZE_ESSENTIAL(e));
#endif
    }
}

static void updateFunc(redisDb *db, robj *ht, robj **argv, vc *t)
{
    vc_update(getCurrent(ht), t);
    server.dirty++;

    rle *e = GET_RLE(db, argv[0], argv[1], 1);
    sds type = argv[2]->ptr;
    sdstolower(type);
    long long value;
    getLongLongFromObject(argv[3], &value);

    listNode *ln;
    listIter li;
    listRewind(e->aset, &li);
    while ((ln = listNext(&li)))
    {
        rl_ase *a = ln->value;
        if (vc_cmp(a->t, t) == VC_LESS) do
            {
#define UPD_NORMAL(T)                \
    if (strcmp(type, #T) == 0)       \
    {                                \
        if (vc_cmp(a->T##_t, t) < 0) \
        {                            \
            vc_update(a->T##_t, t);  \
            a->T##_t->id = t->id;    \
            a->T = value;            \
        }                            \
        break;                       \
    }
                FORALL_NORMAL(UPD_NORMAL);
#undef UPD_NORMAL
#define UPD_BITMAP(T)                  \
    if (strcmp(type, #T) == 0)         \
    {                                  \
        if (vc_cmp(a->T##_t, t) < 0)   \
        {                              \
            vc_update(a->T##_t, t);    \
            a->T##_t->id = t->id;      \
            if (value == 0)            \
                a->property &= ~__##T; \
            else                       \
                a->property |= __##T;  \
        }                              \
        break;                         \
    }
                FORALL_BITMAP(UPD_BITMAP);
#undef UPD_BITMAP
            } while (0);
    }
}

static void removeFunc(redisDb *db, robj *ht, robj **argv, vc *t)
{
    vc_update(getCurrent(ht), t);
    server.dirty++;

    rle *e = GET_RLE(db, argv[0], argv[1], 1);
    int pre_rmv = LOOKUP(e);
    vc *r = vc_dup(t);
    listAddNodeTail(e->rset, r);
#ifdef CRDT_OVERHEAD
    ovhd_inc(RLE_RSE_SIZE);
#endif

    listNode *ln;
    listIter li;
    listRewind(e->aset, &li);
    while ((ln = listNext(&li)))
    {
        rl_ase *a = ln->value;
        if (vc_cmp(a->t, t) == VC_LESS)
        {
            listDelNode(e->aset, ln);
            if (e->value == a) e->value = NULL;
            rl_aseDelete(a);
        }
    }

    if (e->value == NULL)
    {
        listRewind(e->aset, &li);
        while ((ln = listNext(&li)))
        {
            rl_ase *a = ln->value;
            if (e->value == NULL || e->value->t->id < a->t->id) e->value = a;
        }
    }

    if (pre_rmv == 1 && !LOOKUP(e))
    {
        incrLen(ht, -1);
#ifdef CRDT_OVERHEAD
        ovhd_inc(RLE_SIZE_ESSENTIAL(e));
#endif
    }
}

static void notifyLoop(redisDb *db, robj *ht)
{
    list *ops = getOps(ht);
    vc *current = getCurrent(ht);
    int changed;
    do
    {
        changed = 0;
        listNode *ln;
        listIter li;
        listRewind(ops, &li);
        while ((ln = listNext(&li)))
        {
            rl_cmd *cmd = ln->value;
            if (vc_causally_ready(current, cmd->t))
            {
                changed = 1;
                switch (cmd->type)
                {
                    case RLADD:
                        insertFunc(db, ht, cmd->argv, cmd->t);
                        break;
                    case RLUPDATE:
                        updateFunc(db, ht, cmd->argv, cmd->t);
                        break;
                    case RLREM:
                        removeFunc(db, ht, cmd->argv, cmd->t);
                        break;
                    default:
                        serverPanic("unknown rzset cmd type.");
                }
                listDelNode(ops, ln);
                rl_cmdDelete(cmd);
                break;
            }
        }
    } while (changed);
}

void rlinsertCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(9);
            CHECK_CONTAINER_TYPE(OBJ_HASH);
            for (int i = 5; i < 9; ++i)
                CHECK_ARG_TYPE_INT(c->argv[i]);
            rle *pre = GET_RLE(c->db, c->argv[1], c->argv[2], 0);
            if (pre == NULL)
            {
                sdstolower(c->argv[2]->ptr);
                if (strcmp(c->argv[2]->ptr, "null") != 0)
                {
                    sds errs =
                        sdscatfmt(sdsempty(), "-No pre element %S in the list.\r\n", c->argv[2]->ptr);
                    addReplySds(c, errs);
                    return;
                }
            }
            rle *e = GET_RLE(c->db, c->argv[1], c->argv[3], 1);
            if (LOOKUP(e))
            {
                addReply(c, shared.ele_exist);
                return;
            }
            if (e->oid == NULL)
            {
                leid *left = pre == NULL ? NULL : pre->pos_id;
                leid *right;
                if (pre == NULL)
                {
                    rle *head = getHead(GET_LIST_HT(argv, 1));
                    right = head == NULL ? NULL : head->pos_id;
                }
                else
                    right = pre->next == NULL ? NULL : pre->next->pos_id;
                vc *cur = getCurrent(GET_LIST_HT(argv, 1));
                leid *id = constructLeid(left, right, cur);
                RARGV_ADD_SDS(leidToSds(id));
                leidFree(id);
            }
            else
                RARGV_ADD_SDS(leidToSds(e->pos_id));
            RARGV_ADD_SDS(vc_now(getCurrent(GET_LIST_HT(argv, 1))));
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            robj *ht = GET_LIST_HT(rargv, 1);
            vc *current = getCurrent(ht);
            if (vc_causally_ready(current, t))
            {
                insertFunc(c->db, ht, &c->rargv[1], t);
                vc_delete(t);
                notifyLoop(c->db, ht);
            }
            else
            {
                rl_cmd *cmd = rl_cmdNew(RLADD, c, t);
                listAddNodeTail(getOps(ht), cmd);
            }
    CRDT_END
}

void rlupdateCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(5);
            CHECK_CONTAINER_TYPE(OBJ_HASH);
            CHECK_ARG_TYPE_INT(c->argv[4]);
            rle *e = GET_RLE(c->db, c->argv[1], c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }
            RARGV_ADD_SDS(vc_now(getCurrent(GET_LIST_HT(argv, 1))));
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            robj *ht = GET_LIST_HT(rargv, 1);
            vc *current = getCurrent(ht);
            if (vc_causally_ready(current, t))
            {
                updateFunc(c->db, ht, &c->rargv[1], t);
                vc_delete(t);
                notifyLoop(c->db, ht);
            }
            else
            {
                rl_cmd *cmd = rl_cmdNew(RLUPDATE, c, t);
                listAddNodeTail(getOps(ht), cmd);
            }
    CRDT_END
}

void rlremCommand(client *c)
{
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC(3);
            CHECK_CONTAINER_TYPE(OBJ_HASH);
            rle *e = GET_RLE(c->db, c->argv[1], c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }
            RARGV_ADD_SDS(vc_now(getCurrent(GET_LIST_HT(argv, 1))));
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            robj *ht = GET_LIST_HT(rargv, 1);
            vc *current = getCurrent(ht);
            if (vc_causally_ready(current, t))
            {
                removeFunc(c->db, ht, &c->rargv[1], t);
                vc_delete(t);
                notifyLoop(c->db, ht);
            }
            else
            {
                rl_cmd *cmd = rl_cmdNew(RLREM, c, t);
                listAddNodeTail(getOps(ht), cmd);
            }
    CRDT_END
}

void rllenCommand(client *c)
{
    robj *o = lookupKeyReadOrReply(c, c->argv[1], shared.czero);
    if (o == NULL || checkType(c, o, OBJ_HASH)) return;
    addReplyLongLong(c, getLen(o));
}

void rllistCommand(client *c)
{
    robj *o = lookupKeyReadOrReply(c, c->argv[1], shared.emptyarray);
    if (o == NULL || checkType(c, o, OBJ_HASH)) return;
    addReplyArrayLen(c, getLen(o));
    rle *e = getHead(o);
    while (e != NULL)
    {
        if (LOOKUP(e))
        {
            addReplyArrayLen(c, 6);
            addReplyBulkCBuffer(c, e->oid, sdslen(e->oid));
            addReplyBulkCBuffer(c, e->content, sdslen(e->content));
#define TMP_ACTION(p) addReplyBulkLongLong(c, e->value->p);
            FORALL_NORMAL(TMP_ACTION);
#undef TMP_ACTION
            addReplyBulkLongLong(c, e->value->property);
        }
        e = e->next;
    }
}

#ifdef CRDT_OPCOUNT
void rlopcountCommand(client *c) { addReplyLongLong(c, op_count_get()); }
#endif

#ifdef CRDT_OVERHEAD
void rloverheadCommand(client *c) { addReplyLongLong(c, ovhd_get()); }
#endif
