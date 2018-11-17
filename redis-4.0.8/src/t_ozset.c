//
// Created by user on 18-9-16.
//

#include "server.h"

#ifdef Z_OVERHEAD
#define SUF_OZETOTAL "ozetotal"
#define SUF_ASET "ozaset"
#define SUF_RSET "ozrset"
static redisDb* cur_db=NULL;
static sds cur_tname=NULL;
#endif

#ifdef RPQ_LOG
#include <sys/time.h>
FILE *ozLog = NULL;
#define check(f)\
    do\
    {\
        if((f)==NULL)\
            (f)=fopen("ozlog","a");\
    }while(0)
static long currentTime()
{
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return  tv.tv_sec*1000000+tv.tv_usec;
}
#endif

#define ORI_RPQ_TABLE_SUFFIX "_ozets_"
#define LOOKUP(e) (listLength((e)->aset) != 0)
#define SCORE(e) (((e)->innate == NULL ? 0 : (e)->innate->x)\
                  + ((e)->acquired == NULL ? 0 : (e)->acquired->inc))

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

#define OZESIZE(e) (sizeof(oze) + 2 * sizeof(list) \
                    + listLength((e)->aset) * (sizeof(ase) + sizeof(ct) + sizeof(listNode)) \
                    + listLength((e)->rset) * (sizeof(ct) + sizeof(listNode)));

sds aseToSds(ase *a)
{
    return sdscatprintf(sdsempty(), "(%d,%d),%f,%f,%f", a->t->x, a->t->id, a->x, a->inc, a->count);
}

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
#ifdef Z_OVERHEAD
robj* _get_ovhd_count(redisDb* db,sds tname,const char* suf)
{
    robj *logname = createObject(OBJ_STRING, sdscat(sdsdup(tname), suf));
    robj* o;
    if((o=lookupKeyWrite(db,logname) )== NULL)
    {
        o=createObject(OBJ_STRING,0);
        o->encoding = OBJ_ENCODING_INT;
        dbAdd(db,logname,o);
    }
    decrRefCount(logname);
    return o;
}
void inc_ovhd_count(redisDb* db,sds tname,const char* suf,long i)
{
    robj* o=_get_ovhd_count(db,tname,suf);
    o->ptr= (void *) ((long)o->ptr + i);
}
long get_ovhd_count(redisDb* db,sds tname,const char* suf)
{
    robj* o=_get_ovhd_count(db,tname,suf);
    return (long) (o->ptr);
}
#endif


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
            if (delete)
            {
                listDelNode(e->aset, ln);
#ifdef Z_OVERHEAD
                inc_ovhd_count(cur_db,cur_tname,SUF_ASET,-1);
#endif
            }
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
#ifdef Z_OVERHEAD
            inc_ovhd_count(cur_db,cur_tname,SUF_RSET,-1);
#endif
            return a;
        }
    }
    return NULL;
}

// 下面函数对自己参数 t 没有所有权
ase *add_ase(oze *e, ct *t)
{
    ase *a = zmalloc(sizeof(ase));
    a->t = ct_dup(t);
    a->inc = 0;
    a->count = 0;
    listAddNodeTail(e->aset, a);
#ifdef Z_OVERHEAD
    inc_ovhd_count(cur_db,cur_tname,SUF_ASET,1);
#endif
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
    if (e->acquired == a)
        return 1;
    if (e->acquired == NULL || e->acquired->count < a->count ||
        (e->acquired->count == a->count && ct_cmp(e->acquired->t, a->t) < 0))
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
#ifdef Z_OVERHEAD
    inc_ovhd_count(cur_db,cur_tname,SUF_RSET,1);
#endif
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
#ifdef Z_OVERHEAD
        inc_ovhd_count(cur_db,cur_tname,SUF_OZETOTAL,1);
#endif
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
#ifdef Z_OVERHEAD
    PRE_SET;
#endif
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

#ifdef RPQ_LOG
            check(ozLog);
            fprintf(ozLog, "%ld,%s,%s %s %s\n", currentTime(),
                    (char *) c->argv[0]->ptr,
                    (char *) c->argv[1]->ptr,
                    (char *) c->argv[2]->ptr,
                    (char *) c->argv[3]->ptr);
            fflush(ozLog);
#endif

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
                zsetAdd(zset, SCORE(e), c->rargv[2]->ptr, &flags, NULL);
            }
            zfree(t);
            server.dirty++;
    CRDT_END
}

void ozincrbyCommand(client *c)
{
#ifdef Z_OVERHEAD
    PRE_SET;
#endif
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

#ifdef RPQ_LOG
            check(ozLog);
            fprintf(ozLog, "%ld,%s,%s %s %s\n", currentTime(),
                    (char *) c->argv[0]->ptr,
                    (char *) c->argv[1]->ptr,
                    (char *) c->argv[2]->ptr,
                    (char *) c->argv[3]->ptr);
            fflush(ozLog);
#endif

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
                zsetAdd(zset, SCORE(e), c->rargv[2]->ptr, &flags, NULL);
            }
            server.dirty++;
    CRDT_END
}

void ozremCommand(client *c)
{
#ifdef Z_OVERHEAD
    PRE_SET;
#endif
    CRDT_BEGIN
        CRDT_ATSOURCE
            if (checkArgcAndZsetType(c, 3)) return;
            oze *e = ozeHTGet(c->db, c->argv[1], c->argv[2], 0);
            if (e == NULL || !LOOKUP(e))
            {
                addReply(c, shared.ele_nexist);
                return;
            }

#ifdef RPQ_LOG
            check(ozLog);
            fprintf(ozLog, "%ld,%s,%s %s\n", currentTime(),
                    (char *) c->argv[0]->ptr,
                    (char *) c->argv[1]->ptr,
                    (char *) c->argv[2]->ptr);
            fflush(ozLog);
#endif

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
                    zsetAdd(zset, SCORE(e), c->rargv[2]->ptr, &flags, NULL);
                }
                else
                {
                    zsetDel(zset, c->rargv[2]->ptr);
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
#ifdef RPQ_LOG
        check(ozLog);
        fprintf(ozLog, "%ld,%s,%s,NONE\n", currentTime(),
                (char *) c->argv[0]->ptr,
                (char *) c->argv[1]->ptr);
        fflush(ozLog);
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
        check(ozLog);
        if (vstr == NULL)
            fprintf(ozLog, "%ld,%s,%s,%ld %f\n", currentTime(),
                    (char *) c->argv[0]->ptr,
                    (char *) c->argv[1]->ptr,
                    (long) vlong, zzlGetScore(sptr));
        else
        {
            char *temp = zmalloc(sizeof(char) * (vlen + 1));
            for (unsigned int i = 0; i < vlen; ++i)
                temp[i] = vstr[i];
            temp[vlen] = '\0';
            fprintf(ozLog, "%ld,%s,%s,%s %f\n", currentTime(),
                    (char *) c->argv[0]->ptr,
                    (char *) c->argv[1]->ptr,
                    temp, zzlGetScore(sptr));
            zfree(temp);
        }
        fflush(ozLog);
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
        check(ozLog);
        fprintf(ozLog, "%ld,%s,%s,%s %f\n", currentTime(),
                (char *) c->argv[0]->ptr,
                (char *) c->argv[1]->ptr,
                ele, ln->score);
        fflush(ozLog);
#endif
    }
    else
    {
        serverPanic("Unknown sorted set encoding");
    }
}

void ozestatusCommand(client *c)
{
    oze *e = ozeHTGet(c->db, c->argv[1], c->argv[2], 0);
    if (e == NULL)
    {
        addReply(c, shared.emptymultibulk);
        return;
    }

    unsigned long len = 6 + listLength(e->aset) + listLength(e->rset);
    addReplyMultiBulkLen(c, len);

    addReplyBulkSds(c, sdscatprintf(sdsempty(), "current:%d", e->current));

    addReplyBulkSds(c, sdsnew("innate,acquired:"));
    if (e->innate == NULL)
        addReply(c, shared.emptybulk);
    else
        addReplyBulkSds(c, aseToSds(e->innate));
    if (e->acquired == NULL)
        addReply(c, shared.emptybulk);
    else
        addReplyBulkSds(c, aseToSds(e->acquired));

    listNode *ln;
    listIter li;

    addReplyBulkSds(c, sdsnew("Add Set:"));
    listRewind(e->aset, &li);
    while ((ln = listNext(&li)))
    {
        ase *a = ln->value;
        addReplyBulkSds(c, aseToSds(a));
    }

    addReplyBulkSds(c, sdsnew("Remove Set:"));
    listRewind(e->rset, &li);
    while ((ln = listNext(&li)))
    {
        ct *a = ln->value;
        addReplyBulkSds(c, ctToSds(a));
    }
}

/* Actually the hash set used here to store oze structures is not necessary.
 * We can store oze in the zset, for it's whether ziplist or dict+skiplist.
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
 * key -> pointer that point to metadata (oze*)
 *
 * the metadata contains score information
 * overall the metadata overhead is size used by oze
 * */
#ifdef Z_OVERHEAD
void ozoverheadCommand(client *c)
{
    PRE_SET;
    long long size = get_ovhd_count(cur_db,cur_tname,SUF_OZETOTAL)*(sizeof(oze) + 2 * sizeof(list) )
                    + get_ovhd_count(cur_db,cur_tname,SUF_ASET) * (sizeof(ase) + sizeof(ct) + sizeof(listNode))
                    + get_ovhd_count(cur_db,cur_tname,SUF_RSET) * (sizeof(ct) + sizeof(listNode));
    addReplyLongLong(c, size);
}
#else
void ozoverheadCommand(client *c)
{
    robj *htname = createObject(OBJ_STRING, sdscat(sdsdup(c->argv[1]->ptr), ORI_RPQ_TABLE_SUFFIX));
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
        oze *e = *(oze **) value;
        size += OZESIZE(e);
        sdsfree(value);
    }
    hashTypeReleaseIterator(hi);
    addReplyLongLong(c, size);
    /*
    if (ht->encoding == OBJ_ENCODING_ZIPLIST)
    {
        // Not implemented. We show the overhead calculation method:
        // size += (size of the ziplist structure itself) + (size of keys and values);
        // Iterate the ziplist to get each oze* e;
        // size += OZESIZE(e);
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
            oze *e = *(oze **) value;
            size += OZESIZE(e);
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
