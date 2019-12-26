//
// Created by user on 18-10-6.
//

#include "server.h"
#include "RWFramework.h"

#ifdef RW_OVERHEAD
#define SUF_RZETOTAL "rzetotal"
static redisDb *cur_db = NULL;
static sds cur_tname = NULL;
#endif

#ifdef COUNT_OPS
static int rcount = 0;
#endif

#ifdef RPQ_LOG
static FILE *rzLog = NULL;
#define check(f)\
    do\
    {\
        if((f)==NULL)\
            (f)=fopen("rzlog","a");\
    }while(0)
#endif

#define RW_RPQ_TABLE_SUFFIX "_rzets_"
#define RZESIZE(e) (sizeof(rze) + sizeof(vc) + CURRENT(e)->size * sizeof(int))

typedef struct RW_RPQ_element
{
    reh header;
    double innate;
    double acquired;
    //list *ops;
} rze;

//#define RZADD 0
//#define RZINCBY 1
//typedef struct unready_command
//{
//    int type;
//    robj *tname, *element;
//    double value;
//    vc *t;
//} ucmd;

//int rzeSize(rze *e)
//{
//    int vcsize = sizeof(vc) + e->current->size * sizeof(int);
//    int size = sizeof(rze) + vcsize + sizeof(list)
//               + listLength(e->ops) * (sizeof(listNode) + sizeof(ucmd) + 2 * sizeof(robj) + vcsize);
//    listNode *ln;
//    listIter li;
//    listRewind(e->ops, &li);
//    while ((ln = listNext(&li)))
//    {
//        ucmd *cmd = ln->value;
//        size += sdsAllocSize(cmd->tname->ptr) + sdsAllocSize(cmd->element->ptr);
//    }
//    return size;
//}

//sds ucmdToSds(ucmd *cmd)
//{
//    char *tp[] = {"RZADD", "RZINCBY"};
//    sds vc_s = VCToSds(cmd->t);
//    sds s = sdscatprintf(sdsempty(), "%s %s %s %f %s",
//                         tp[cmd->type],
//                         (char *) cmd->tname->ptr,
//                         (char *) cmd->element->ptr,
//                         cmd->value, vc_s);
//    sdsfree(vc_s);
//    return s;
//}

reh *rzeNew()
{
    rze *e = zmalloc(sizeof(rze));
    REH_INIT(e);
    e->innate = 0;
    e->acquired = 0;
    //e->ops = listCreate();
    return (reh *) e;
}

#define SCORE(e) ((e)->innate+(e)->acquired)
/*
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

rze *rzeHTGet(redisDb *db, robj *tname, robj *key, int create)
{
    robj *ht = getInnerHT(db, tname->ptr, RW_RPQ_TABLE_SUFFIX, create);
    if (ht == NULL)return NULL;
    robj *value = hashTypeGetValueObject(ht, key->ptr);
    rze *e;
    if (value == NULL)
    {
        if (!create)return NULL;
        e = (rze *) rzeNew();
        hashTypeSet(ht, key->ptr, sdsnewlen(&e, sizeof(rze *)), HASH_SET_TAKE_VALUE);
#ifdef RW_OVERHEAD
        inc_ovhd_count(cur_db, cur_tname, SUF_RZETOTAL, 1);
#endif
    }
    else
    {
        e = *(rze **) (value->ptr);
        decrRefCount(value);
    }
    return e;
}
*/

#ifndef RW_OVERHEAD
#define GET_RZE(arg_t, create)\
(rze *) rehHTGet(c->db, c->arg_t[1], RW_RPQ_TABLE_SUFFIX, c->arg_t[2], create, rzeNew)
#else
#define RZE_HT_GET(arg_t,create)\
(rze *) rehHTGet(c->db, c->arg_t[1], RW_RPQ_TABLE_SUFFIX, c->arg_t[2], create, rzeNew, cur_db, cur_tname, SUF_RZETOTAL)
#endif

/*
// no memory free
void insertFunc(rze *e, redisDb *db, robj *tname, robj *element, double value, vc *t)
{
    if (!insertCheck(e, t))return;
    e->aid = t->id;
    e->innate = value;
    robj *zset = getZsetOrCreate(db, tname, element);
    int flags = ZADD_NONE;
    zsetAdd(zset, SCORE(e), element->ptr, &flags, NULL);
    server.dirty++;
}

void increaseFunc(rze *e, redisDb *db, robj *tname, robj *element, double value, vc *t)
{
    if (!increaseCheck(e, t))return;
    e->acquired += value;
    robj *zset = getZsetOrCreate(db, tname, element);
    int flags = ZADD_NONE;
    zsetAdd(zset, SCORE(e), element->ptr, &flags, NULL);
    server.dirty++;
}
*/

// This doesn't free t.
static void removeFunc(client *c, rze *e, vc *t)
{
    if (removeCheck((reh *) e, t))
    {
        REH_RMV_FUNC(e, t);
        e->acquired = 0;
        e->innate = 0;
        robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
        zsetDel(zset, c->rargv[2]->ptr);
        server.dirty++;
        //notifyLoop(e, c->db);
    }
}

/*
void notifyLoop(rze *e, redisDb *db)
{
    listNode *ln;
    listIter li;
    listRewind(e->ops, &li);
    while ((ln = listNext(&li)))
    {
        ucmd *cmd = ln->value;
        if (readyCheck(e, cmd->t))
        {
            switch (cmd->type)
            {
                case RZADD:
                    insertFunc(e, db, cmd->tname, cmd->element, cmd->value, cmd->t);
                    break;
                case RZINCBY:
                    increaseFunc(e, db, cmd->tname, cmd->element, cmd->value, cmd->t);
                    break;
                default:
                    serverPanic("unknown rzset cmd type.");
            }
            listDelNode(e->ops, ln);
            ucmdDelete(cmd);
        }
    }

*/



void rzaddCommand(client *c)
{
#ifdef RW_OVERHEAD
    PRE_SET;
#endif
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_ZSET, 4);
            CHECK_ARG_TYPE_DOUBLE(c->argv[3]);
            rze *e = GET_RZE(argv, 1);
            PREPARE_PRECOND_ADD(e);

#ifdef RPQ_LOG
                check(rzLog);
                fprintf(rzLog, "%ld,%s,%s %s %s\n", currentTime(),
                        (char *) c->argv[0]->ptr,
                        (char *) c->argv[1]->ptr,
                        (char *) c->argv[2]->ptr,
                        (char *) c->argv[3]->ptr);
                fflush(rzLog);
#endif
            ADD_CR_NON_RMV(e);
        CRDT_EFFECT
#ifdef COUNT_OPS
            rcount++;
#endif
            double v;
            getDoubleFromObject(c->rargv[3], &v);
            vc *t = CR_GET_LAST;
            rze *e = GET_RZE(rargv, 1);
            /*
            if (readyCheck(e, t))
            {
                insertFunc(e, c->db, c->rargv[1], c->rargv[2], v, t);
                deleteVC(t);
            }
            else
            {
                ucmd *cmd = ucmdNew(RZADD, c->rargv[1], c->rargv[2], v, t);
                listAddNodeTail(e->ops, cmd);
            }
             */
            removeFunc(c, e, t);
            if (insertCheck((reh *) e, t))
            {
                PID(e) = t->id;
                e->innate = v;
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                int flags = ZADD_NONE;
                zsetAdd(zset, SCORE(e), c->rargv[2]->ptr, &flags, NULL);
                server.dirty++;
            }
            deleteVC(t);
    CRDT_END
}

void rzincrbyCommand(client *c)
{
#ifdef RW_OVERHEAD
    PRE_SET;
#endif
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_ZSET, 4);
            CHECK_ARG_TYPE_DOUBLE(c->argv[3]);
            rze *e = GET_RZE(argv, 0);
            PREPARE_PRECOND_NON_ADD(e);

#ifdef RPQ_LOG
                check(rzLog);
                fprintf(rzLog, "%ld,%s,%s %s %s\n", currentTime(),
                        (char *) c->argv[0]->ptr,
                        (char *) c->argv[1]->ptr,
                        (char *) c->argv[2]->ptr,
                        (char *) c->argv[3]->ptr);
                fflush(rzLog);
#endif

            ADD_CR_NON_RMV(e);
        CRDT_EFFECT
#ifdef COUNT_OPS
            rcount++;
#endif
            double v;
            getDoubleFromObject(c->rargv[3], &v);
            vc *t = CR_GET_LAST;
            rze *e = GET_RZE(rargv, 1);
            /*
            if (readyCheck(e, t))
            {
                increaseFunc(e, c->db, c->rargv[1], c->rargv[2], v, t);
                deleteVC(t);
            }
            else
            {
                ucmd *cmd = ucmdNew(RZINCBY, c->rargv[1], c->rargv[2], v, t);
                listAddNodeTail(e->ops, cmd);
            }
            */
            removeFunc(c, e, t);
            if (updateCheck((reh *) e, t))
            {
                e->acquired += v;
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                int flags = ZADD_NONE;
                zsetAdd(zset, SCORE(e), c->rargv[2]->ptr, &flags, NULL);
                server.dirty++;
            }
            deleteVC(t);
    CRDT_END
}

void rzremCommand(client *c)
{
#ifdef RW_OVERHEAD
    PRE_SET;
#endif
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_ZSET, 3);
            rze *e = GET_RZE(argv, 0);
            PREPARE_PRECOND_NON_ADD(e);

#ifdef RPQ_LOG
                check(rzLog);
                fprintf(rzLog, "%ld,%s,%s %s\n", currentTime(),
                        (char *) c->argv[0]->ptr,
                        (char *) c->argv[1]->ptr,
                        (char *) c->argv[2]->ptr);
                fflush(rzLog);
#endif
            ADD_CR_RMV(e);
        CRDT_EFFECT
#ifdef COUNT_OPS
            rcount++;
#endif
            rze *e = GET_RZE(rargv, 1);
            vc *t = CR_GET_LAST;
            /*
            if (removeCheck(e, t))
            {
                updateVC(e->current, t);
                e->aid = -1;
                e->acquired = 0;
                e->innate = 0;
                robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
                zsetDel(zset, c->rargv[2]->ptr);
                server.dirty++;
                notifyLoop(e, c->db);
            }
            */
            removeFunc(c, e, t);
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
#ifdef RPQ_LOG
        check(rzLog);
        fprintf(rzLog, "%ld,%s,%s,NONE\n", currentTime(),
                (char *) c->argv[0]->ptr,
                (char *) c->argv[1]->ptr);
        fflush(rzLog);
#endif
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

#ifdef RPQ_LOG
        check(rzLog);
        if (vstr == NULL)
            fprintf(rzLog, "%ld,%s,%s,%ld %f\n", currentTime(),
                    (char *) c->argv[0]->ptr,
                    (char *) c->argv[1]->ptr,
                    (long) vlong, zzlGetScore(sptr));
        else
        {
            char *temp = zmalloc(sizeof(char) * (vlen + 1));
            for (unsigned int i = 0; i < vlen; ++i)
                temp[i] = vstr[i];
            temp[vlen] = '\0';
            fprintf(rzLog, "%ld,%s,%s,%s %f\n", currentTime(),
                    (char *) c->argv[0]->ptr,
                    (char *) c->argv[1]->ptr,
                    temp, zzlGetScore(sptr));
            zfree(temp);
        }
        fflush(rzLog);
#endif
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
#ifdef RPQ_LOG
        check(rzLog);
        fprintf(rzLog, "%ld,%s,%s,%s %f\n", currentTime(),
                (char *) c->argv[0]->ptr,
                (char *) c->argv[1]->ptr,
                ele, ln->score);
        fflush(rzLog);
#endif
    }
    else
    {
        serverPanic("Unknown sorted set encoding");
    }
}

void rzestatusCommand(client *c)
{
    rze *e = GET_RZE(argv, 0);
    if (e == NULL)
    {
        addReply(c, shared.emptymultibulk);
        return;
    }

    //unsigned long len = 6 + listLength(e->ops);
    //addReplyMultiBulkLen(c, len);
    addReplyMultiBulkLen(c, 5);

    addReplyBulkSds(c, sdscatprintf(sdsempty(), "innate:%f", e->innate));
    addReplyBulkSds(c, sdscatprintf(sdsempty(), "acquired:%f", e->acquired));
    addReplyBulkSds(c, sdscatprintf(sdsempty(), "add id:%d", PID(e)));

    addReplyBulkSds(c, sdsnew("current:"));
    addReplyBulkSds(c, VCToSds(CURRENT(e)));

    /*
    addReplyBulkSds(c, sdsnew("unready commands:"));
    listNode *ln;
    listIter li;
    listRewind(e->ops, &li);
    while ((ln = listNext(&li)))
    {
        ucmd *cmd = ln->value;
        addReplyBulkSds(c, ucmdToSds(cmd));
    }
     */
}

#ifdef COUNT_OPS

void rzopcountCommand(client *c)
{
    addReplyLongLong(c, rcount);
}

#endif

/* Actually the hash set used here to store rze structures is not necessary.
 * We can store rze in the zset, for it's whether ziplist or dict+skiplist.
 * We use the hash set here for fast implementing our CRDT Algorithms.
 * We may optimize our implementation by not using the hash set and using
 * zset's own dict instead in the future.
 * As for metadata overhead calculation, we here do it as if we have done
 * such optimization. The commented area is the overhead if we take the
 * hash set into account.
 *
 * optimized:
 * zset:
 * key -> score(double)
 * --->
 * key -> pointer that point to metadata (rze*)
 *
 * the metadata contains score information
 * overall the metadata overhead is size used by rze
 * */
#ifdef RW_OVERHEAD

void rzoverheadCommand(client *c)
{
    PRE_SET;
    long long size = get_ovhd_count(cur_db, cur_tname, SUF_RZETOTAL) *
                     (sizeof(rze) + sizeof(vc) + server.p2p_count * sizeof(int));
    addReplyLongLong(c, size);
}

#else

void rzoverheadCommand(client *c)
{
    robj *htname = createObject(OBJ_STRING, sdscat(sdsdup(c->argv[1]->ptr), RW_RPQ_TABLE_SUFFIX));
    robj *ht = lookupKeyRead(c->db, htname);
    long long size = 0;

    /*
     * The overhead for database to store the hash set information.
     * sds temp = sdsdup(htname->ptr);
     * size += sizeof(dictEntry) + sizeof(robj) + sdsAllocSize(temp);
     * sdsfree(temp);
     */

    decrRefCount(htname);
    if (ht == NULL)
    {
        addReplyLongLong(c, 0);
        return;
    }

    hashTypeIterator *hi = hashTypeInitIterator(ht);
    while (hashTypeNext(hi) != C_ERR)
    {
        sds value = hashTypeCurrentObjectNewSds(hi, OBJ_HASH_VALUE);
        rze *e = *(rze **) value;
        //size += rzeSize(e);
        size += RZESIZE(e);
        sdsfree(value);
    }
    hashTypeReleaseIterator(hi);
    addReplyLongLong(c, size);
    /*
    if (ht->encoding == OBJ_ENCODING_ZIPLIST)
    {
        // Not implemented. We show the overhead calculation method:
        // size += (size of the ziplist structure itself) + (size of keys and values);
        // Iterate the ziplist to get each rze* e;
        // size += rzeSize(e);
    }
    else if (ht->encoding == OBJ_ENCODING_HT)
    {
        dict *d = ht->ptr;
        size += sizeof(dict) + sizeof(dictType) + (d->ht[0].size + d->ht[1].size) * sizeof(dictEntry *)
                + (d->ht[0].used + d->ht[1].used) * sizeof(dictEntry);

        dictIterator *di = dictGetIterator(d);
        dictEntry *de;
        while ((de = dictNext(di)) != NULL)
        {
            sds key = dictGetKey(de);
            sds value = dictGetVal(de);
            size += sdsAllocSize(key) + sdsAllocSize(value);
            rze *e = *(rze **) value;
            size += rzeSize(e);
        }
        dictReleaseIterator(di);
    }
    else
    {
        serverPanic("Unknown hash encoding");
    }
    */
}

#endif
