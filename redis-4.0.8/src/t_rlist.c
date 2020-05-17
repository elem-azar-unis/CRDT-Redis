//
// Created by user on 20-04-12.
//
#include "server.h"
#include "RWFramework.h"
#include "list_basics.h"

#define DEFINE_NPR(p) int p;
#define DEFINE_A(p) vc *p##_t;
typedef struct rlist_aset_element
{
    vc *t;
    FORALL(DEFINE_A);
    FORALL_NPR(DEFINE_NPR);
    int property;
} rl_ase;
#undef DEFINE_NPR
#undef DEFINE_A

rl_ase *rl_aseNew(vc *t)
{
    rl_ase *a = zmalloc(sizeof(rl_ase));
    a->t = duplicateVC(t);
#define TMP_ACTION(p) a->p##_t = duplicateVC(t);
    FORALL(TMP_ACTION);
#undef TMP_ACTION
    return a;
}

enum rl_cmd_type
{
    RLADD, RLUPDATE, RLREM
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
    for (int i = 0; i < cmd->argc; ++i)
    {
        cmd->argv[i] = c->rargv[i + 1];
        incrRefCount(cmd->argv[i]);
    }
    return cmd;
}

void rl_cmdDelete(rl_cmd *cmd)
{
    deleteVC(cmd->t);
    for (int i = 0; i < cmd->argc; ++i)
        decrRefCount(cmd->argv[i]);
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
        RWFHT_SET(ht, hname, list*, e);
    }
    else
    {
        e = *(list **) (value->ptr);
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

    struct rw_list_element *prev;
    struct rw_list_element *next;
} rle;

rle *rleNew()
{
    rle *e = zmalloc(sizeof(rle));
    e->oid = NULL;
    e->pos_id = NULL;
    e->content = NULL;

    e->value = NULL;
    e->aset = listCreate();
    e->rset = listCreate();

    e->prev = NULL;
    e->next = NULL;
    return e;
}

rle *rleHTGet(redisDb *db, robj *tname, robj *key, int create)
{
    robj *ht = getInnerHT(db, tname, NULL, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key->ptr);
    rle *e;
    if (value == NULL)
    {
        if (!create)return NULL;
        e = rleNew();
        hashTypeSet(ht, key->ptr, sdsnewlen(&e, sizeof(rle *)), HASH_SET_TAKE_VALUE);
#ifdef RW_OVERHEAD
        inc_ovhd_count(cur_db, cur_tname, SUF_RZETOTAL, 1);
#endif
    }
    else
    {
        e = *(rle **) (value->ptr);
        decrRefCount(value);
    }
    return e;
}

// doesn't free t, doesn't own t
static void insertFunc(client *c, robj *ht, robj **argv, vc *t)
{
    updateVC(getCurrent(ht), t);
    // TODO
}

static void updateFunc(client *c, robj *ht, robj **argv, vc *t)
{
    updateVC(getCurrent(ht), t);
    // TODO
}

static void removeFunc(client *c, robj *ht, robj **argv, vc *t)
{
    updateVC(getCurrent(ht), t);
    // TODO
}

static void notifyLoop(client *c, robj *ht)
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
            if (causally_ready(current, cmd->t))
            {
                changed = 1;
                switch (cmd->type)
                {
                    case RLADD:
                        insertFunc(c, ht, cmd->argv, cmd->t);
                        break;
                    case RLUPDATE:
                        updateFunc(c, ht, cmd->argv, cmd->t);
                        break;
                    case RLREM:
                        removeFunc(c, ht, cmd->argv, cmd->t);
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
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_HASH, 9);
            for (int i = 5; i < 9; ++i)
                CHECK_ARG_TYPE_INT(c->argv[i]);
            rle *pre = rleHTGet(c->db,c->argv[1],c->argv[2],0);
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
            rle*e=rleHTGet(c->db, c->argv[1], c->argv[3], 1);
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
            RARGV_ADD_SDS(nowVC(getCurrent(GET_LIST_HT(argv, 1))));
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            robj *ht = GET_LIST_HT(rargv, 1);
            vc *current = getCurrent(ht);
            if (causally_ready(current, t))
            {
                insertFunc(c, ht, &c->rargv[1], t);
                deleteVC(t);
                notifyLoop(c, ht);
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
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_HASH, 5);
            CHECK_ARG_TYPE_INT(c->argv[4]);
            rle* e=rleHTGet(c->db,c->argv[1],c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }
            RARGV_ADD_SDS(nowVC(getCurrent(GET_LIST_HT(argv, 1))));
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            robj *ht = GET_LIST_HT(rargv, 1);
            vc *current = getCurrent(ht);
            if (causally_ready(current, t))
            {
                updateFunc(c, ht, &c->rargv[1], t);
                deleteVC(t);
                notifyLoop(c, ht);
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
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_HASH, 3);
            rle* e=rleHTGet(c->db,c->argv[1],c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }
            RARGV_ADD_SDS(nowVC(getCurrent(GET_LIST_HT(argv, 1))));
        CRDT_EFFECT
            vc *t = CR_GET_LAST;
            robj *ht = GET_LIST_HT(rargv, 1);
            vc *current = getCurrent(ht);
            if (causally_ready(current, t))
            {
                removeFunc(c, ht, &c->rargv[1], t);
                deleteVC(t);
                notifyLoop(c, ht);
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
    robj *o = lookupKeyReadOrReply(c, c->argv[1], shared.emptymultibulk);
    if (o == NULL || checkType(c, o, OBJ_HASH)) return;
    addReplyMultiBulkLen(c, getLen(o));
    rle *e = getHead(o);
    while (e != NULL)
    {
        if (LOOKUP(e))
        {
            addReplyMultiBulkLen(c, 6);
            addReplyBulkCBuffer(c, e->oid, sdslen(e->oid));
            addReplyBulkCBuffer(c, e->content, sdslen(e->content));
#define TMP_ACTION(p) addReplyBulkLongLong(c, e->value->p);
            FORALL_NPR(TMP_ACTION);
#undef TMP_ACTION
            addReplyBulkLongLong(c, e->value->property);
        }
        e = e->next;
    }
}
