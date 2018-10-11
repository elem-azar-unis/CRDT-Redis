//
// Created by user on 18-10-6.
//

#include "server.h"

#define RW_RPQ_TABLE_SUFFIX "_rzets_"

typedef struct RW_RPQ_element
{
    double innate;
    double acquired;
    int aid;
    vc *current;
    list *ops;
} rze;

#define RZADD 0
#define RZINCBY 1
typedef struct unready_command
{
    int type;
    robj *tname, *element;
    double value;
    vc *t;
} ucmd;

rze *rzeNew()
{
    rze *e = zmalloc(sizeof(rze));
    e->current = l_newVC;
    e->innate = 0;
    e->acquired = 0;
    e->aid = -1;
    e->ops = listCreate();
    return e;
}

// 获得 t 所有权
ucmd *ucmdNew(int type, robj *tname, robj *element, double value, vc *t)
{
    ucmd *cmd = zmalloc(sizeof(ucmd));
    cmd->type = type;
    cmd->tname = tname;
    incrRefCount(tname);
    cmd->element = element;
    incrRefCount(element);
    cmd->value = value;
    cmd->t = t;
    return cmd;
}

void ucmdDelete(ucmd *cmd)
{
    decrRefCount(cmd->tname);
    decrRefCount(cmd->element);
    deleteVC(cmd->t);
    zfree(cmd);
}

int insertCheck(rze *e, vc *t)
{
    if (equalVC(t, e->current) == 0)return 0;
    return e->aid < t->id;
}

int increaseCheck(rze *e, vc *t)
{
    return equalVC(t, e->current);
}

int removeCheck(rze *e, vc *t)
{
    int id = t->id;
    if (e->current->vector[id] < t->vector[id])return 1;
    return compareVC(t, e->current) != CLOCK_LESS;
}

int readyCheck(rze *e, vc *t)
{
    int *current = e->current->vector;
    int *next = t->vector;
    int equal = 1;
    for (int i = 0; i < t->size; ++i)
    {
        if (current[i] > next[i])
            return 1;
        if (current[i] < next[i])
            equal = 0;
    }
    return equal;
}

#define LOOKUP(e) ((e)->aid >= 0)
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

// 下面两个不进行内存释放
void insertFunc(rze *e,redisDb *db, robj *tname, robj *element, double value, vc *t)
{
    if (!insertCheck(e, t))return;
    e->aid = t->id;
    e->innate = value;
    robj *zset = getZsetOrCreate(db, tname, element);
    int flags = ZADD_NONE;
    zsetAdd(zset, SCORE(e), element->ptr, &flags, NULL);
    server.dirty++;
}

void increaseFunc(rze *e,redisDb *db, robj *tname, robj *element, double value, vc *t)
{
    if (!increaseCheck(e, t))return;
    e->acquired += value;
    robj *zset = getZsetOrCreate(db, tname, element);
    int flags = ZADD_NONE;
    zsetAdd(zset, SCORE(e), element->ptr, &flags, NULL);
    server.dirty++;
}

void notifyLoop(rze *e,redisDb *db)
{
    listNode *ln;
    listIter li;
    listRewind(e->ops, &li);
    while ((ln = listNext(&li)))
    {
        ucmd *cmd = ln->value;
        if(readyCheck(e,cmd->t))
        {
            switch (cmd->type)
            {
                case RZADD:
                    insertFunc(e,db,cmd->tname,cmd->element,cmd->value,cmd->t);
                    break;
                case RZINCBY:
                    increaseFunc(e,db,cmd->tname,cmd->element,cmd->value,cmd->t);
                    break;
                default:
                    serverPanic("unknown rzset cmd type.");
            }
            listDelNode(e->ops,ln);
            ucmdDelete(cmd);
        }
    }
}

sds now(rze *e)
{
    l_increaseVC(e->current);
    return VCToSds(e->current);
}

void rzaddCommand(client *c)
{
    CRDT_BEGIN
        CRDT_ATSOURCE
            if (checkArgcAndZsetType(c, 4)) return;
            double v;
            if (getDoubleFromObjectOrReply(c, c->argv[3], &v, NULL) != C_OK)
                return;
            rze *e = rzeHTGet(c->db, c->argv[1], c->argv[2], 1);
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

            c->rargv[4] = createObject(OBJ_STRING, VCToSds(e->current));
            addReply(c, shared.ok);
        CRDT_DOWNSTREAM
            double v;
            getDoubleFromObject(c->rargv[3], &v);
            vc *t = SdsToVC(c->rargv[4]->ptr);
            rze *e = rzeHTGet(c->db, c->rargv[1], c->rargv[2], 1);
            if (readyCheck(e, t))
            {
                insertFunc(e,c->db, c->rargv[1], c->rargv[2], v, t);
                deleteVC(t);
            }
            else
            {
                ucmd *cmd = ucmdNew(RZADD, c->rargv[1], c->rargv[2], v, t);
                listAddNodeTail(e->ops, cmd);
            }
    CRDT_END
}

void rzincrbyCommand(client *c)
{
    CRDT_BEGIN
        CRDT_ATSOURCE
            if (checkArgcAndZsetType(c, 4)) return;
            double v;
            if (getDoubleFromObjectOrReply(c, c->argv[3], &v, NULL) != C_OK)
                return;
            rze *e = rzeHTGet(c->db, c->argv[1], c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }

            PREPARE_RARGC(5);
            COPY_ARG_TO_RARG(0, 0);
            COPY_ARG_TO_RARG(1, 1);
            COPY_ARG_TO_RARG(2, 2);
            COPY_ARG_TO_RARG(3, 3);

            c->rargv[4] = createObject(OBJ_STRING, VCToSds(e->current));
            addReply(c, shared.ok);
        CRDT_DOWNSTREAM
            double v;
            getDoubleFromObject(c->rargv[3], &v);
            vc *t = SdsToVC(c->rargv[4]->ptr);
            rze *e = rzeHTGet(c->db, c->rargv[1], c->rargv[2], 1);
            if (readyCheck(e, t))
            {
                increaseFunc(e,c->db, c->rargv[1], c->rargv[2], v, t);
                deleteVC(t);
            }
            else
            {
                ucmd *cmd = ucmdNew(RZINCBY, c->rargv[1], c->rargv[2], v, t);
                listAddNodeTail(e->ops, cmd);
            }
    CRDT_END
}

void rzremCommand(client *c)
{
    CRDT_BEGIN
        CRDT_ATSOURCE
            if (checkArgcAndZsetType(c, 3)) return;
            rze *e = rzeHTGet(c->db, c->argv[1], c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }

            PREPARE_RARGC(4);
            COPY_ARG_TO_RARG(0, 0);
            COPY_ARG_TO_RARG(1, 1);
            COPY_ARG_TO_RARG(2, 2);

            c->rargv[3] = createObject(OBJ_STRING, now(e));
            addReply(c, shared.ok);
        CRDT_DOWNSTREAM
            rze *e = rzeHTGet(c->db, c->rargv[1], c->rargv[2], 1);
            vc *t = SdsToVC(c->rargv[3]->ptr);
            if (removeCheck(e, t))
            {
                updateVC(e->current, t);
                e->aid = -1;
                e->acquired = 0;
                e->innate = 0;
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                zsetDel(zset, c->rargv[2]->ptr);
                server.dirty++;
                notifyLoop(e,c->db);
            }
            deleteVC(t);
    CRDT_END
}

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