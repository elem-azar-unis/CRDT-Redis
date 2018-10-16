//
// Created by user on 18-4-12.
//
#include "server.h"


int connectWithReplica(char *ip, int port, int *fd)
{
    *fd = anetTcpNonBlockBestEffortBindConnect(NULL, ip, port, NET_FIRST_BIND_ADDR);
    if (*fd == -1)
    {
        serverLog(LL_WARNING, "Unable to connect to REPLICA[%s:%d]: %s",
                  ip, port, strerror(errno));
        return C_ERR;
    }

    return C_OK;
}

void repltestCommand(client* c)
{
    /* REPLICATE is not allowed in cluster mode. */
    if (server.cluster_enabled)
    {
        addReplyError(c, "SLAVEOF not allowed in cluster mode.");
        return;
    }

    c->flags |= CLIENT_REPLICA;
    c->authenticated = 1;
    listAddNodeTail(server.replicas, c);
    serverLog(LL_NOTICE, "Replica handshake received.");

    long id,size;
    getLongFromObjectOrReply(c, c->argv[1], &size, "invalid replica size.");
    getLongFromObjectOrReply(c, c->argv[2], &id, "invalid replica id.");
    server.p2p_count=(int)size;
    server.p2p_id=(int)id;
}

void replicateCommand(client *c)
{
    /* REPLICATE is not allowed in cluster mode. */
    if (server.cluster_enabled)
    {
        addReplyError(c, "SLAVEOF not allowed in cluster mode.");
        return;
    }

    if (!strcasecmp(c->argv[1]->ptr, "replica") &&
        !strcasecmp(c->argv[2]->ptr, "handshake"))
    {
        c->flags |= CLIENT_REPLICA;
        c->authenticated = 1;
        listAddNodeTail(server.replicas, c);
        serverLog(LL_NOTICE, "Replica handshake received.");
        return;
    }

    long id,size;
    getLongFromObjectOrReply(c, c->argv[1], &size, "invalid replica size.");
    getLongFromObjectOrReply(c, c->argv[2], &id, "invalid replica id.");
    server.p2p_count=(int)size;
    server.p2p_id=(int)id;

    for (int i = 3; i < c->argc; i += 2)
    {
        int fd;
        long port;
        getLongFromObjectOrReply(c, c->argv[i + 1], &port, "invalid port number.");
        if (connectWithReplica(c->argv[i]->ptr, (int) port, &fd) == C_OK)
            serverLog(LL_NOTICE, "Connected to REPLICA %s:%d",
                      (char *) c->argv[i]->ptr, (int) port);
        else
        {
            addReplySds(c, sdsnew("-Something wrong connecting replicas.\r\n"));
            return;
        }
        client *r = createClient(fd);
        addReply(r, shared.replhandshake);
        r->authenticated = 1;
        r->flags |= CLIENT_REPLICA;
        listAddNodeTail(server.replicas, r);
    }
    addReply(c, shared.ok);
}

void replicationBroadcast(list *replicas, int dictid, robj **argv, int argc)
{
    listNode *ln;
    listIter li;
    int j, selected = 0;
    char llstr[LONG_STR_SIZE];

    if (listLength(replicas) == 0)return;

    /* Send SELECT command to every replica if needed. */
    if (server.p2p_seldb != dictid)
    {
        selected = 1;
        robj *selectcmd;

        /* For a few DBs we have pre-computed SELECT command. */
        if (dictid >= 0 && dictid < PROTO_SHARED_SELECT_CMDS)
        {
            selectcmd = shared.select[dictid];
        } else
        {
            int dictid_len;

            dictid_len = ll2string(llstr, sizeof(llstr), dictid);
            selectcmd = createObject(OBJ_STRING,
                                     sdscatprintf(sdsempty(),
                                                  "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
                                                  dictid_len, llstr));
        }

        /* Send it to replicas. */
        listRewind(replicas, &li);
        while ((ln = listNext(&li)))
        {
            client *replica = ln->value;
            // if (replica->replstate == SLAVE_STATE_WAIT_BGSAVE_START) continue;
            replica->flags |= CLIENT_REPLICA_MESSAGE;
            addReply(replica, shared.multi);
            addReply(replica, selectcmd);
            replica->flags &= ~CLIENT_REPLICA_MESSAGE;
        }

        if (dictid < 0 || dictid >= PROTO_SHARED_SELECT_CMDS)
            decrRefCount(selectcmd);
    }
    server.p2p_seldb = dictid;

    /* Write the command to every replica. */
    listRewind(replicas, &li);
    while ((ln = listNext(&li)))
    {
        client *replica = ln->value;

        replica->flags |= CLIENT_REPLICA_MESSAGE;

        /* Don't feed replica that are still waiting for BGSAVE to start
        if (replica->replstate == SLAVE_STATE_WAIT_BGSAVE_START) continue;*/

        /* Add the multi bulk length. */
        addReplyMultiBulkLen(replica, argc);

        /* Finally any additional argument that was not stored inside the
         * static buffer if any (from j to argc). */
        for (j = 0; j < argc; j++)
            addReplyBulk(replica, argv[j]);

        if(selected) addReply(replica, shared.exec);

        replica->flags &= ~CLIENT_REPLICA_MESSAGE;
    }
}
