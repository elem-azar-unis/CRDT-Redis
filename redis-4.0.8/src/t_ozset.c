//
// Created by user on 18-9-16.
//

#include "server.h"

#define ORI_RPQ_TABLE_SUFFIX "_ozets_"
#define LOOKUP(e) listLength((e)->aset) != 0

typedef struct clock_tag
{
    int x;
    int id;
} ct;

typedef struct aset_element
{
    ct *t;
    double x;
    double inc;
    double count;
} ase;

typedef struct ORI_RPQ_element
{
    int current;
    ase *innate;
    ase *acquired;
    list *aset;
    list *rset;
} oze;

int ct_cmp(ct *t1, ct *t2)
{
    if (t1->id != t2->id)return t1->id - t2->id;
    return t1->x - t2->x;
}

ct *ct_new(oze *e)
{
    ct *t = zmalloc(sizeof(ct));
    t->id = PID;
    t->x = e->current;
    e->current++;
    return t;
}

ct *ct_dup(ct *t)
{
    ct *n = zmalloc(sizeof(ct));
    n->x = t->x;
    n->id = t->id;
    return n;
}

sds ctToSds(const ct *t)
{
    return sdscatprintf(sdsempty(), "%d,%d", t->x, t->id);
}

ct *sdsToCt(sds s)
{
    ct *t = zmalloc(sizeof(ct));
    sscanf(s, "%d,%d", &t->x, &t->id);
    return t;
}

oze *ozeNew()
{
    oze *e = zmalloc(sizeof(oze));
    e->current = 0;
    e->innate = NULL;
    e->acquired = NULL;
    e->aset = listCreate();
    e->rset = listCreate();
    return e;
}

ase *asetGet(oze *e, ct *t, int delete)
{
    listNode *ln;
    listIter li;
    listRewind(e->aset, &li);
    while ((ln = listNext(&li)))
    {
        ase *a = ln->value;
        if (ct_cmp(t, a->t) == 0)
        {
            if (delete) listDelNode(e->aset, ln);
            return a;
        }
    }
    return NULL;
}

ct *rsetGet(oze *e, ct *t, int delete)
{
    listNode *ln;
    listIter li;
    listRewind(e->rset, &li);
    while ((ln = listNext(&li)))
    {
        ct *a = ln->value;
        if (ct_cmp(t, a) == 0)
        {
            if (delete) listDelNode(e->rset, ln);
            return a;
        }
    }
    return NULL;
}

double score(oze *e)
{
    double x = 0;
    if (e->innate != NULL)x = e->innate->x;
    if (e->acquired != NULL)x += e->acquired->inc;
    return x;
}

// 下面函数对自己参数 t 没有所有权
ase *add_ase(oze *e, ct *t)
{
    ase *a = zmalloc(sizeof(ase));
    a->t = ct_dup(t);
    a->inc = 0;
    a->count = 0;
    listAddNodeTail(e->aset, a);
    return a;
}

int update_innate_value(oze *e, ct *t, double v)
{
    if (rsetGet(e, t, 0) != NULL)return 0;
    ase *a = asetGet(e, t, 0);
    if (a == NULL) a = add_ase(e, t);
    a->x = v;
    if (e->innate == NULL || ct_cmp(e->innate->t, a->t) < 0)
    {
        e->innate = a;
        return 1;
    }
    return 0;
}

int update_acquired_value(oze *e, ct *t, double v)
{
    if (rsetGet(e, t, 0) != NULL)return 0;
    ase *a = asetGet(e, t, 0);
    if (a == NULL) a = add_ase(e, t);
    a->inc += v;
    a->count += (v > 0) ? v : -v;
    if (e->acquired->count < a->count || (e->acquired->count == a->count && ct_cmp(e->acquired->t, a->t) < 0))
    {
        e->acquired = a;
        return 1;
    }
    return 0;
}

// 没有整理
void remove_tag(oze *e, ct *t)
{
    if (rsetGet(e, t, 0) != NULL)return;
    ct *nt = ct_dup(t);
    listAddNodeTail(e->rset, nt);
    ase *a;
    if ((a = asetGet(e, t, 1)) != NULL)
    {
        if (e->innate == a)e->innate = NULL;
        if (e->acquired == a)e->acquired = NULL;
        zfree(a->t);
        zfree(a);
    }
}

void resort(oze *e)
{
    if (e->innate != NULL && e->acquired != NULL)return;

    listNode *ln;
    listIter li;
    listRewind(e->aset, &li);

    while ((ln = listNext(&li)))
    {
        ase *a = ln->value;
        if (e->innate == NULL || ct_cmp(e->innate->t, a->t) < 0)
            e->innate = a;
        if (e->acquired == NULL || e->acquired->count < a->count ||
            (e->acquired->count == a->count && ct_cmp(e->acquired->t, a->t) < 0))
            e->acquired = a;
    }
}

robj *getInnerHT(redisDb *db, sds tname, const char *suffix, int create)
{
    robj *htname = createObject(OBJ_STRING, sdscat(sdsdup(tname), suffix));
    robj *ht = lookupKeyRead(db, htname);
    if (create && ht == NULL)
    {
        ht = createHashObject();
        dbAdd(db, htname, ht);
    }
    decrRefCount(htname);
    return ht;
}

oze *ozeHTGet(redisDb *db, robj *tname, robj *key, int create)
{
    robj *ht = getInnerHT(db, tname->ptr, ORI_RPQ_TABLE_SUFFIX, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key->ptr);
    oze *e;
    if (value == NULL)
    {
        if (!create)return NULL;
        e = ozeNew();
        hashTypeSet(ht, key->ptr, sdsnewlen(&e, sizeof(oze *)), HASH_SET_TAKE_VALUE);
    }
    else
    {
        e = *(oze **) (value->ptr);
        decrRefCount(value);
    }
    return e;
}

int checkArgcAndZsetType(client *c, int max)
{
    if (c->argc > max)
    {
        addReply(c, shared.syntaxerr);
        return 1;
    }
    robj *zset = lookupKeyRead(c->db, c->argv[1]);
    if (zset != NULL && zset->type != OBJ_ZSET)
    {
        addReply(c, shared.wrongtypeerr);
        return 1;
    }
    return 0;
}

robj *getZsetOrCreate(redisDb *db, robj *zset_name, robj *element_name)
{
    robj *zobj = lookupKeyWrite(db, zset_name);
    if (zobj == NULL)
    {
        if (server.zset_max_ziplist_entries == 0 ||
            server.zset_max_ziplist_value < sdslen(element_name->ptr))
        {
            zobj = createZsetObject();
        }
        else
        {
            zobj = createZsetZiplistObject();
        }
        dbAdd(db, zset_name, zobj);
    }
    return zobj;
}

void ozaddCommand(client *c)
{
    CRDT_BEGIN
        CRDT_ATSOURCE
            if (checkArgcAndZsetType(c, 4)) return;
            double v;
            if (getDoubleFromObjectOrReply(c, c->argv[3], &v, NULL) != C_OK)
                return;
            oze *e = ozeHTGet(c->db, c->argv[1], c->argv[2], 1);
            if (LOOKUP(e))
            {
                addReply(c, shared.ele_exist);
                return;
            }

            PREPARE_RARGC(5);
            COPY_ARG_TO_RARG(0, 0);
            COPY_ARG_TO_RARG(1, 1);
            COPY_ARG_TO_RARG(2, 2);
            COPY_ARG_TO_RARG(3, 3);

            ct *t = ct_new(e);
            c->rargv[4] = createObject(OBJ_STRING, ctToSds(t));
            zfree(t);
            addReply(c, shared.ok);
        CRDT_DOWNSTREAM
            double v;
            getDoubleFromObject(c->rargv[3], &v);
            ct *t = sdsToCt(c->rargv[4]->ptr);
            oze *e = ozeHTGet(c->db, c->rargv[1], c->rargv[2], 1);
            if (update_innate_value(e, t, v))
            {
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                int flags = ZADD_NONE;
                zsetAdd(zset, score(e), c->rargv[2]->ptr, &flags, NULL);
            }
            zfree(t);
            server.dirty++;
    CRDT_END
}

void ozincrbyCommand(client *c)
{
    CRDT_BEGIN
        CRDT_ATSOURCE
            if (checkArgcAndZsetType(c, 4)) return;
            double v;
            if (getDoubleFromObjectOrReply(c, c->argv[3], &v, NULL) != C_OK)
                return;
            oze *e = ozeHTGet(c->db, c->argv[1], c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }

            PREPARE_RARGC(4 + listLength(e->aset));
            COPY_ARG_TO_RARG(0, 0);
            COPY_ARG_TO_RARG(1, 1);
            COPY_ARG_TO_RARG(2, 2);
            COPY_ARG_TO_RARG(3, 3);

            int i = 4;
            listNode *ln;
            listIter li;
            listRewind(e->aset, &li);
            while ((ln = listNext(&li)))
            {
                ase *a = ln->value;
                c->rargv[i] = createObject(OBJ_STRING, ctToSds(a->t));
                i++;
            }
            addReply(c, shared.ok);
        CRDT_DOWNSTREAM
            double v;
            getDoubleFromObject(c->rargv[3], &v);
            oze *e = ozeHTGet(c->db, c->rargv[1], c->rargv[2], 1);
            int changed = 0;
            for (int i = 4; i < c->rargc; i++)
            {
                ct *t = sdsToCt(c->rargv[i]->ptr);
                changed += update_acquired_value(e, t, v);
                zfree(t);
            }
            if (changed)
            {
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                int flags = ZADD_NONE;
                zsetAdd(zset, score(e), c->rargv[2]->ptr, &flags, NULL);
            }
            server.dirty++;
    CRDT_END
}

void ozremCommand(client *c)
{
    CRDT_BEGIN
        CRDT_ATSOURCE
            if (checkArgcAndZsetType(c, 3)) return;
            oze *e = ozeHTGet(c->db, c->argv[1], c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }

            PREPARE_RARGC(3 + listLength(e->aset));
            COPY_ARG_TO_RARG(0, 0);
            COPY_ARG_TO_RARG(1, 1);
            COPY_ARG_TO_RARG(2, 2);

            int i = 3;
            listNode *ln;
            listIter li;
            listRewind(e->aset, &li);
            while ((ln = listNext(&li)))
            {
                ase *a = ln->value;
                c->rargv[i] = createObject(OBJ_STRING, ctToSds(a->t));
                i++;
            }
            addReply(c, shared.ok);
        CRDT_DOWNSTREAM
            oze *e = ozeHTGet(c->db, c->rargv[1], c->rargv[2], 1);
            for (int i = 3; i < c->rargc; i++)
            {
                ct *t = sdsToCt(c->rargv[i]->ptr);
                remove_tag(e, t);
                zfree(t);
            }
            if (e->innate == NULL || e->acquired == NULL)
            {
                resort(e);
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                if (LOOKUP(e))
                {
                    int flags = ZADD_NONE;
                    zsetAdd(zset, score(e), c->rargv[2]->ptr, &flags, NULL);
                }
                else
                {
                    zsetDel(zset,c->rargv[2]->ptr);
                }
            }
            server.dirty++;
    CRDT_END
}

void ozscoreCommand(client *c)
{
    robj *key = c->argv[1];
    robj *zobj;
    double score;

    if ((zobj = lookupKeyReadOrReply(c, key, shared.nullbulk)) == NULL ||
        checkType(c, zobj, OBJ_ZSET))
        return;

    if (zsetScore(zobj, c->argv[2]->ptr, &score) == C_ERR)
    {
        addReply(c, shared.nullbulk);
    }
    else
    {
        addReplyDouble(c, score);
    }
}

void ozmaxCommand(client *c)
{
    robj *zobj;
    if ((zobj = lookupKeyReadOrReply(c, c->argv[1], shared.emptymultibulk)) == NULL
        || checkType(c, zobj, OBJ_ZSET))
        return;
    if (zsetLength(zobj) == 0)
    {
        addReply(c, shared.emptymultibulk);
        return;
    }
    addReplyMultiBulkLen(c, 2);
    if (zobj->encoding == OBJ_ENCODING_ZIPLIST)
    {
        unsigned char *zl = zobj->ptr;
        unsigned char *eptr, *sptr;
        unsigned char *vstr;
        unsigned int vlen;
        long long vlong;

        eptr = ziplistIndex(zl, -2);
        sptr = ziplistNext(zl, eptr);

        serverAssertWithInfo(c, zobj, eptr != NULL && sptr != NULL);
        serverAssertWithInfo(c, zobj, ziplistGet(eptr, &vstr, &vlen, &vlong));
        if (vstr == NULL)
            addReplyBulkLongLong(c, vlong);
        else
            addReplyBulkCBuffer(c, vstr, vlen);
        addReplyDouble(c, zzlGetScore(sptr));
    }
    else if (zobj->encoding == OBJ_ENCODING_SKIPLIST)
    {
        zset *zs = zobj->ptr;
        zskiplist *zsl = zs->zsl;
        zskiplistNode *ln = zsl->tail;
        serverAssertWithInfo(c, zobj, ln != NULL);
        sds ele = ln->ele;
        addReplyBulkCBuffer(c, ele, sdslen(ele));
        addReplyDouble(c, ln->score);
    }
    else
    {
        serverPanic("Unknown sorted set encoding");
    }
}
