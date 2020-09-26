//
// Created by user on 19-10-30.
//

#ifndef REDIS_CRDT_H
#define REDIS_CRDT_H

#include "CRDT_exp.h"

#define ADDITIONAL_CAPACITY 4

/**
 * Prepare rargc and rargv.
 * - copy the current argv to rargv
 * */
#define INIT_RARGV                                         \
    do                                                     \
    {                                                      \
        c->rargc = c->argc;                                \
        c->rargv = zmalloc(sizeof(robj *) * __capacity__); \
        for (int _i_ = 0; _i_ < c->argc; _i_++)            \
        {                                                  \
            c->rargv[_i_] = c->argv[_i_];                  \
            incrRefCount(c->rargv[(_i_)]);                 \
        }                                                  \
    } while (0)

/**
 * The following 2 macros are used to add additional arg to rargv.
 * ! Warning: When you use them, there is no regret to broadcast the rargv.
 * */

// Add one additional arg to rargv type robj*. Will NOT delete the obj.
#define _RARGV_ADD(obj)                                                    \
    do                                                                    \
    {                                                                     \
        if (!c->rargv) INIT_RARGV;                                        \
        if (c->rargc == __capacity__)                                     \
        {                                                                 \
            __capacity__ = 2 * __capacity__ - c->argc;                    \
            c->rargv = zrealloc(c->rargv, sizeof(robj *) * __capacity__); \
        }                                                                 \
        c->rargv[c->rargc] = obj;                                         \
        c->rargc++;                                                       \
    } while (0)

// Add one additional arg to rargv, type sds. Take the ownership of the sds.
#define RARGV_ADD_SDS(str) _RARGV_ADD(createObject(OBJ_STRING, str))

// Check the argc and the container type. You may use this at the start of the prepare phase.

#define CHECK_ARGC(_max)                   \
    do                                     \
    {                                      \
        if (c->argc > _max || c->argc < 2) \
        {                                  \
            addReply(c, shared.syntaxerr); \
            return;                        \
        }                                  \
    } while (0)

#define CHECK_CONTAINER_TYPE(_type)                  \
    do                                               \
    {                                                \
        robj *_o = lookupKeyRead(c->db, c->argv[1]); \
        if (_o != NULL && _o->type != _type)         \
        {                                            \
            addReply(c, shared.wrongtypeerr);        \
            return;                                  \
        }                                            \
    } while (0)

// Check the type of the arg, int / double.

#define CHECK_ARG_TYPE_INT(_obj)                                                                 \
    do                                                                                           \
    {                                                                                            \
        long long __temp__;                                                                      \
        if (getLongLongFromObjectOrReply(c, _obj, &__temp__, "value is not an integer") != C_OK) \
            return;                                                                              \
    } while (0)

#define CHECK_ARG_TYPE_DOUBLE(_obj)                                                               \
    do                                                                                            \
    {                                                                                             \
        double __temp__;                                                                          \
        if (getDoubleFromObjectOrReply(c, _obj, &__temp__, "value is not a valid float") != C_OK) \
            return;                                                                               \
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
 *             // - check correctness of
 *                  1. the format of the command from the cliend and
 *                  2. the type of the targeted CRDT
 *                  using CHECK_ARGC, CHECK_CONTAINER_TYPE,
 *                  CHECK_ARG_TYPE_INT and CHECK_ARG_TYPE_DOUBLE macros
 *             // - check the precondition of the prepare phase, read the local state
 *             // - add additional parameters to broadcast using RARGV_ADD_SDS macro
 *             // Will automatically reply an "OK" message to the client if
 *             // the prepare phase ends successfully.
 *         CRDT_EFFECT
 *             // Get the parameters from c->rargv, c->rargc is the number of paremeters.
 *             // The order of the parameters in c->rargv is: The parameters of the initial client
 *             // calling this command, then the additional parameters from the prepare phase,
 *             // in the order of calling RARGV_ADD_SDS macros.
 *             // Trust the correctness of these parameters. Here we do no further check.
 *
 *             // Then do the effect phase of the command according to the algorithm.
 *
 *             // Remember to free the variables you malloced here.
 *     CRDT_END
 * }
 *
 * */
#define REPLICATION_MODE listLength(server.replicas) || server.p2p_count

#define CRDT_BEGIN        \
    if (REPLICATION_MODE) \
    {                     \
        TIME_ISTR_BEGIN;

#define CRDT_PREPARE                  \
    if (!(c->flags & CLIENT_REPLICA)) \
    {                                 \
        int __capacity__ = c->argc + ADDITIONAL_CAPACITY;

#define CRDT_EFFECT            \
    addReply(c, shared.ok);    \
    if (!c->rargv) INIT_RARGV; \
    LOG_ISTR_PRE;              \
    }                          \
    {                          \
        OPCOUNT_ISTR;          \
        LOG_ISTR_EFF;

#define CRDT_END                                                       \
    }                                                                  \
    if (c->flags & CLIENT_REPLICA_MESSAGE) { addReply(c, shared.ok); } \
    TIME_ISTR_END;                                                     \
    return;                                                            \
    }                                                                  \
    else                                                               \
    {                                                                  \
        addReply(c, shared.notreplerr);                                \
        return;                                                        \
    }

#endif  // REDIS_CRDT_H
