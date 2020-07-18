//
// Created by user on 19-10-4.
//
#include "RWFramework.h"

robj *getInnerHT(redisDb *db, robj *tname, const char *suffix, int create)
{
    robj *htname;
    if (suffix != NULL)
        htname = createObject(OBJ_STRING, sdscat(sdsdup(tname->ptr), suffix));
    else
        htname = createObject(OBJ_STRING, sdsdup(tname->ptr));
    robj *ht = lookupKeyRead(db, htname);
    if (create && ht == NULL)
    {
        ht = createHashObject();
        dbAdd(db, htname, ht);
    }
    decrRefCount(htname);
    return ht;
}

reh *rehHTGet(redisDb *db, robj *tname, const char *suffix, robj *key, int create,
              rehNew_func_t rehNew_p)
{
    robj *ht = getInnerHT(db, tname, suffix, create);
    if (ht == NULL) return NULL;
    robj *value = hashTypeGetValueObject(ht, key->ptr);
    reh *e;
    if (value == NULL)
    {
        if (!create) return NULL;
        e = (*rehNew_p)();
        RWFHT_SET(ht, key->ptr, reh *, e);
    }
    else
    {
        e = *(reh **)(value->ptr);
        decrRefCount(value);
    }
    return e;
}