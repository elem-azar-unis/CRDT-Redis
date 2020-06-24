//
// Created by user on 19-10-30.
//

#ifndef REDIS_4_0_8_CRDT_H
#define REDIS_4_0_8_CRDT_H

#include "CRDT_exp.h"

#define PREPARE_RARGC(n)                          \
    do                                            \
    {                                             \
        c->rargc = (n);                           \
        c->rargv = zmalloc(sizeof(robj *) * (n)); \
    } while (0)

#define COPY_ARG_TO_RARG(a, r)        \
    do                                \
    {                                 \
        c->rargv[(r)] = c->argv[(a)]; \
        incrRefCount(c->rargv[(r)]);  \
    } while (0)

typedef struct prepare_rargv_vector
{
    int size;
    int capacity;
    void **vector; //actually here is robj** vector;
} rvector;

#define INITIAL_CAPACITY 4
#define RVCT_PREPARE                                                  \
    do                                                                \
    {                                                                 \
        __rvct__.vector = zmalloc(INITIAL_CAPACITY * sizeof(robj *)); \
        __rvct__.size = 0;                                            \
        __rvct__.capacity = INITIAL_CAPACITY;                         \
    } while (0)

/* add one additional arg, type robj*, will not delete the obj */
#define RARGV_ADD(obj)                                                      \
    do                                                                      \
    {                                                                       \
        if (__rvct__.size == __rvct__.capacity)                             \
        {                                                                   \
            __rvct__.capacity *= 2;                                         \
            __rvct__.vector = zrealloc(__rvct__.vector, __rvct__.capacity); \
        }                                                                   \
        __rvct__.vector[__rvct__.size] = obj;                               \
        __rvct__.size++;                                                    \
    } while (0)

/* add one additional arg, type sds, will delete the sds */
#define RARGV_ADD_SDS(str) RARGV_ADD(createObject(OBJ_STRING, str))

/*
 * Prepare rargc and rargv.
 * - copy the current argv to rargv
 * - add the additional args in __rcvt__ to rargv
 * */
#define RVCT_BROADCAST                                      \
    do                                                      \
    {                                                       \
        PREPARE_RARGC(c->argc + __rvct__.size);             \
        for (int _i_ = 0; _i_ < c->argc; _i_++)             \
            COPY_ARG_TO_RARG(_i_, _i_);                     \
        for (int _i_ = 0; _i_ < __rvct__.size; _i_++)       \
            c->rargv[c->argc + _i_] = __rvct__.vector[_i_]; \
        zfree(__rvct__.vector);                             \
    } while (0)

// check the argc and the container type.
// you may use this at the start of the prepare phase.
#define CHECK_ARGC_AND_CONTAINER_TYPE(_type, _max)  \
    do                                              \
    {                                               \
        if (c->argc > _max || c->argc < 2)          \
        {                                           \
            addReply(c, shared.syntaxerr);          \
            return;                                 \
        }                                           \
        robj *o = lookupKeyRead(c->db, c->argv[1]); \
        if (o != NULL && o->type != _type)          \
        {                                           \
            addReply(c, shared.wrongtypeerr);       \
            return;                                 \
        }                                           \
    } while (0)

// Check the type of the arg, int / double

#define CHECK_ARG_TYPE_INT(_obj)                                             \
    do                                                                       \
    {                                                                        \
        long long __temp__;                                                  \
        if (getLongLongFromObjectOrReply(c, _obj, &__temp__,                 \
                                         "value is not an integer") != C_OK) \
            return;                                                          \
    } while (0)

#define CHECK_ARG_TYPE_DOUBLE(_obj)                                           \
    do                                                                        \
    {                                                                         \
        double __temp__;                                                      \
        if (getDoubleFromObjectOrReply(c, _obj, &__temp__,                    \
                                       "value is not a valid float") != C_OK) \
            return;                                                           \
    } while (0)

/*
 * The core macros for the CRDT framework. You may use these macros to write your CRDT operations.
 * Here is an example:
 *
 * void exampleCommand(client* c)
 * {
 *     CRDT_BEGIN
 *         CRDT_PREPARE
 *             // Do the prepare phase, such as:
 *             // - check correctness of 1. the format of the command from the cliend and
 *                  2. the type of the targeted CRDT using CHECK_ARGC_AND_CONTAINER_TYPE macro
 *             // - check the precondition of the prepare phase, read the local state
 *             // - add additional parameters to broadcast using RARGV_ADD or RARGV_ADD_SDS macros
 *             // Will automatically reply an "OK" message to the client if the prepare phase ends successfully.
 *         CRDT_EFFECT
 *             // Get the parameters from c->rargv, c->rargc is the number of paremeters.
 *             // The order of the parameters in c->rargv is: The parameters of the initial client calling
 *             // this command, then the additional parameters from the prepare phase, in tht order of calling
 *             // RARGV_ADD or RARGV_ADD_SDS macros.
 *             // Trust the correctness of these parameters. Here we do no further check.
 *
 *             // Then do the effect phase of the command according to the algorithm.
 *
 *             // Remember to free the variables you malloc here.
 *     CRDT_END
 * }
 * 
 * */
#define REPLICATION_MODE listLength(server.replicas) || server.p2p_count

#define CRDT_BEGIN        \
    if (REPLICATION_MODE) \
    {

#define CRDT_PREPARE                  \
    if (!(c->flags & CLIENT_REPLICA)) \
    {                                 \
        rvector __rvct__;             \
        RVCT_PREPARE;

#define CRDT_EFFECT         \
    addReply(c, shared.ok); \
    RVCT_BROADCAST;         \
    LOG_ISTR_PRE;           \
    }                       \
    {                       \
        OPCOUNT_ISTR;       \
        LOG_ISTR_EFF;

#define CRDT_END                           \
    }                                      \
    if (c->flags & CLIENT_REPLICA_MESSAGE) \
    {                                      \
        addReply(c, shared.ok);            \
    }                                      \
    return;                                \
    }

#endif //REDIS_4_0_8_CRDT_H
