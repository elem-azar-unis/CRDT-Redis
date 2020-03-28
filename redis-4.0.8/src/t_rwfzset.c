//
// Created by user on 18-10-6.
//

#include "server.h"
#include "RWFramework.h"

#ifdef RW_OVERHEAD
#define SUF_RZETOTAL "rwfzetotal"
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

#define RWF_RPQ_TABLE_SUFFIX "_rwfzets_"
#define RWFZESIZE(e) (sizeof(rwfze) + sizeof(vc) + CURRENT(e)->size * sizeof(int))

typedef struct RWF_RPQ_element
{
    reh header;
    double innate;
    double acquired;
} rwfze;

reh *rwfzeNew()
{
    rwfze *e = zmalloc(sizeof(rwfze));
    REH_INIT(e);
    e->innate = 0;
    e->acquired = 0;
    return (reh *) e;
}

#define SCORE(e) ((e)->innate+(e)->acquired)

#ifndef RW_OVERHEAD
#define GET_RZE(arg_t, create)\
(rwfze *) rehHTGet(c->db, c->arg_t[1], RWF_RPQ_TABLE_SUFFIX, c->arg_t[2], create, rwfzeNew)
#else
#define RZE_HT_GET(arg_t,create)\
(rwfze *) rehHTGet(c->db, c->arg_t[1], RWF_RPQ_TABLE_SUFFIX, c->arg_t[2], create, rwfzeNew, cur_db, cur_tname, SUF_RZETOTAL)
#endif

// This doesn't free t.
static void removeFunc(client *c, rwfze *e, vc *t)
{
    if (removeCheck((reh *) e, t))
    {
        REH_RMV_FUNC(e, t);
        e->acquired = 0;
        e->innate = 0;
        robj *zset = getZsetOrCreate(c->db, c->rargv[1], c->rargv[2]);
        zsetDel(zset, c->rargv[2]->ptr);
        server.dirty++;
    }
}


void rwfzaddCommand(client *c)
{
#ifdef RW_OVERHEAD
    PRE_SET;
#endif
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_ZSET, 4);
            CHECK_ARG_TYPE_DOUBLE(c->argv[3]);
            rwfze *e = GET_RZE(argv, 1);
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
            rwfze *e = GET_RZE(rargv, 1);
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

void rwfzincrbyCommand(client *c)
{
#ifdef RW_OVERHEAD
    PRE_SET;
#endif
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_ZSET, 4);
            CHECK_ARG_TYPE_DOUBLE(c->argv[3]);
            rwfze *e = GET_RZE(argv, 0);
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
            rwfze *e = GET_RZE(rargv, 1);
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

void rwfzremCommand(client *c)
{
#ifdef RW_OVERHEAD
    PRE_SET;
#endif
    CRDT_BEGIN
        CRDT_PREPARE
            CHECK_ARGC_AND_CONTAINER_TYPE(OBJ_ZSET, 3);
            rwfze *e = GET_RZE(argv, 0);
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
            rwfze *e = GET_RZE(rargv, 1);
            vc *t = CR_GET_LAST;
            removeFunc(c, e, t);
            deleteVC(t);
    CRDT_END
}

void rwfzscoreCommand(client *c)
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

void rwfzmaxCommand(client *c)
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

void rwfzestatusCommand(client *c)
{
    rwfze *e = GET_RZE(argv, 0);
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

}

#ifdef COUNT_OPS

void rwfzopcountCommand(client *c)
{
    addReplyLongLong(c, rcount);
}

#endif

/* Actually the hash set used here to store rwfze structures is not necessary.
 * We can store rwfze in the zset, for it's whether ziplist or dict+skiplist.
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
 * key -> pointer that point to metadata (rwfze*)
 *
 * the metadata contains score information
 * overall the metadata overhead is size used by rwfze
 * */
#ifdef RW_OVERHEAD

void rwfzoverheadCommand(client *c)
{
    PRE_SET;
    long long size = get_ovhd_count(cur_db, cur_tname, SUF_RZETOTAL) *
                     (sizeof(rwfze) + sizeof(vc) + server.p2p_count * sizeof(int));
    addReplyLongLong(c, size);
}

#else

void rwfzoverheadCommand(client *c)
{
    robj *htname = createObject(OBJ_STRING, sdscat(sdsdup(c->argv[1]->ptr), RWF_RPQ_TABLE_SUFFIX));
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
        rwfze *e = *(rwfze **) value;
        //size += rzeSize(e);
        size += RWFZESIZE(e);
        sdsfree(value);
    }
    hashTypeReleaseIterator(hi);
    addReplyLongLong(c, size);
    /*
    if (ht->encoding == OBJ_ENCODING_ZIPLIST)
    {
        // Not implemented. We show the overhead calculation method:
        // size += (size of the ziplist structure itself) + (size of keys and values);
        // Iterate the ziplist to get each rwfze* e;
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
            rwfze *e = *(rwfze **) value;
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
