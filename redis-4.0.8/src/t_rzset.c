//
// Created by user on 18-10-6.
//

#include "server.h"

#define RW_RPQ_TABLE_SUFFIX "_rzets_"

typedef struct RW_RPQ_element
{
    vc *current;
    double innate;
    double acquired;
    list *aset;
    list *rset;
    list *ops;
} rze;

#define RZADD 0
#define RZINCBY 1
typedef struct unready_command
{
    int type;
    int argc;
    robj **argv;
} ucmd;

rze *rzeNew()
{
    rze *e = zmalloc(sizeof(rze));
    e->current = l_newVC;
    e->innate = 0;
    e->acquired = 0;
    e->aset = listCreate();
    e->rset = listCreate();
    e->ops = listCreate();
}

ucmd *ucmdNew(client *c, int type)
{
    ucmd *cmd = zmalloc(sizeof(ucmd));
    cmd->type = type;
    cmd->argc = c->rargc - 1;
    cmd->argv = zmalloc(sizeof(robj *) * cmd->argc);
    for (int i = 0; i < cmd->argc; ++i)
    {
        cmd->argv[i]=c->rargv[i+1];
        incrRefCount(cmd->argv[i]);
    }
}

void ucmdDelete(ucmd* cmd)
{
    for (int i = 0; i < cmd->argc; ++i)
    {
        decrRefCount(cmd->argv[i]);
    }
    zfree(cmd->argv);
    zfree(cmd);
}

int insertCheck(rze *e, vc *t)
{
    listNode *ln;
    listIter li;
    listRewind(e->rset, &li);
    while ((ln = listNext(&li)))
    {
        vc *a = ln->value;
        if (compareVC(t, a) != CLOCK_GREATER)
            return 0;
    }
    return 1;
}

int increaseCheck(rze *e, vc *t)
{
    listNode *ln;
    listIter li;

    listRewind(e->rset, &li);
    while ((ln = listNext(&li)))
    {
        vc *a = ln->value;
        if (compareVC(t, a) != CLOCK_GREATER)
            return 0;
    }

    listRewind(e->aset, &li);
    while ((ln = listNext(&li)))
    {
        vc *a = ln->value;
        int c = compareVC(t, a);
        if (CONCURRENT(c))
            return 1;
    }
    return 0;
}

int removeCheck(rze *e, vc *t)
{
    listNode *ln;
    listIter li;

    listRewind(e->rset, &li);
    while ((ln = listNext(&li)))
    {
        vc *a = ln->value;
        if (compareVC(t, a) == CLOCK_LESS)
            return 0;
    }
    return 1;
}

#define LOOKUP(e) (listLength((e)->aset) != 0)
#define SCORE(e) ((e)->innate+(e)->acquired)

rze *rzeHTGet(redisDb *db, robj *tname, robj *key, int create)
{
    robj *ht = getInnerHT(db, tname->ptr, RW_RPQ_TABLE_SUFFIX, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key->ptr);
    rze *e;
    if (value == NULL)
    {
        if (!create)return NULL;
        e = rzeNew();
        hashTypeSet(ht, key->ptr, sdsnewlen(&e, sizeof(rze *)), HASH_SET_TAKE_VALUE);
    }
    else
    {
        e = *(rze **) (value->ptr);
        decrRefCount(value);
    }
    return e;
}

void rzaddCommand(client *c);

void rzincrbyCommand(client *c);

void rzremCommand(client *c);

void rzscoreCommand(client *c)
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

void rzmaxCommand(client *c)
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