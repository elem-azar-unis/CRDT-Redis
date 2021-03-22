//
// Created by user on 18-4-12.
//
#include "server.h"
#include <arpa/inet.h>

static int exp_local = 0;

void get_peer_ip(int fd, char *ip)
{
    socklen_t clilen;
    struct sockaddr_in cliaddr;
    getpeername(fd, (struct sockaddr *)&cliaddr, &clilen);
    inet_ntop(AF_INET, &cliaddr.sin_addr, ip, 32);
}

void replicationHandler(connection *conn)
{
    client *r = createClient(conn);
    if (conn)
        connSetReadHandler(r->conn, readQueryFromClient);
    addReply(r, shared.replhandshake);
    r->authenticated = 1;
    r->flags |= CLIENT_REPLICA;
    r->user = NULL; /* This client can do everything. */
    listAddNodeTail(server.replicas, r);
}

int connectWithReplica(char *ip, int port)
{
    char *source_addr = exp_local ? ip : NET_FIRST_BIND_ADDR;
    connection *conn = connCreateSocket();
    if (connConnect(conn, ip, port, source_addr, replicationHandler) == C_ERR)
    {
        serverLog(LL_WARNING, "Unable to connect to REPLICA[%s:%d]: %s",
                  ip, port, connGetLastError(conn));
        connClose(conn);
        return C_ERR;
    }
    return C_OK;
}

void repltestCommand(client *c)
{
    /* REPLICATE is not allowed in cluster mode. */
    if (server.cluster_enabled)
    {
        addReplyError(c, "SLAVEOF not allowed in cluster mode.");
        return;
    }

    c->flags |= CLIENT_REPLICA;
    c->authenticated = 1;
    if (c->argc == 1)
    {
        listAddNodeTail(server.replicas, c);
        serverLog(LL_NOTICE, "A listening fake replica.");
        c->flags |= CLIENT_REPLICA_MESSAGE;
        addReply(c, shared.ok);
        c->flags &= ~CLIENT_REPLICA_MESSAGE;
    }

    if (c->argc == 3)
    {
        c->flags |= CLIENT_REPLICA_MESSAGE;
        long id, size;
        getLongFromObjectOrReply(c, c->argv[1], &size, "invalid replica size.");
        getLongFromObjectOrReply(c, c->argv[2], &id, "invalid replica id.");
        server.p2p_count = (int)size;
        server.p2p_id = (int)id;
        serverLog(LL_NOTICE, "An instructing fake replica.");
        addReply(c, shared.ok);
    }
}

void replicateCommand(client *c)
{
    /* REPLICATE is not allowed in cluster mode. */
    if (server.cluster_enabled)
    {
        addReplyError(c, "REPLICATE not allowed in cluster mode.");
        return;
    }

    if (!strcasecmp(c->argv[1]->ptr, "replica") &&
        !strcasecmp(c->argv[2]->ptr, "handshake"))
    {
        c->flags |= CLIENT_REPLICA;
        c->authenticated = 1;
        listAddNodeTail(server.replicas, c);
        char _ip[32];
        get_peer_ip(c->conn->fd, _ip);
        serverLog(LL_NOTICE, "Replica handshake received. ip: %s", _ip);
        return;
    }

    long id, size;
    getLongFromObjectOrReply(c, c->argv[1], &size, "invalid replica size.");
    getLongFromObjectOrReply(c, c->argv[2], &id, "invalid replica id.");
    server.p2p_count = (int)size;
    server.p2p_id = (int)id;

    if (c->argc > 3 && !strcasecmp(c->argv[3]->ptr, "exp_local"))
        exp_local = 1;

    for (int i = 3 + exp_local; i < c->argc; i += 2)
    {
        long port;
        getLongFromObjectOrReply(c, c->argv[i + 1], &port, "invalid port number.");
        if (connectWithReplica(c->argv[i]->ptr, (int)port) == C_OK)
        {
            serverLog(LL_NOTICE, "Connected to REPLICA %s:%ld",
                      (char *)(c->argv[i + 1]->ptr), port);
        }
        else
        {
            addReplySds(c, sdsnew("-Something wrong connecting replicas.\r\n"));
            return;
        }
    }
    addReply(c, shared.ok);
}

//void replicationBroadcast(list *replicas, int dictid, robj **argv, int argc)
void replicationBroadcast(list *replicas, robj **argv, int argc)
{
    listNode *ln;
    listIter li;
    int j;
//    int selected = 0;
//    char llstr[LONG_STR_SIZE];

    if (listLength(replicas) == 0)
        return;

    /* Send SELECT command to every replica if needed.
       Need to implement the p2p select command.
       We assume all the replicas always use the same DB and
       not implement the p2p select for now.*/
    // if (server.p2p_seldb != dictid)
    // {
    //     selected = 1;
    //     robj *selectcmd;

    //     /* For a few DBs we have pre-computed SELECT command. */
    //     if (dictid >= 0 && dictid < PROTO_SHARED_SELECT_CMDS)
    //     {
    //         selectcmd = shared.select[dictid];
    //     }
    //     else
    //     {
    //         int dictid_len;

    //         dictid_len = ll2string(llstr, sizeof(llstr), dictid);
    //         selectcmd = createObject(OBJ_STRING,
    //                                  sdscatprintf(sdsempty(),
    //                                               "*2\r\n$6\r\nSELECT\r\n$%d\r\n%s\r\n",
    //                                               dictid_len, llstr));
    //     }

    //     /* Send it to replicas. */
    //     listRewind(replicas, &li);
    //     while ((ln = listNext(&li)))
    //     {
    //         client *replica = ln->value;
    //         // if (replica->replstate == SLAVE_STATE_WAIT_BGSAVE_START) continue;
    //         replica->flags |= CLIENT_REPLICA_MESSAGE;
    //         addReply(replica, shared.multi_cmd);
    //         addReply(replica, selectcmd);
    //         replica->flags &= ~CLIENT_REPLICA_MESSAGE;
    //     }

    //     if (dictid < 0 || dictid >= PROTO_SHARED_SELECT_CMDS)
    //         decrRefCount(selectcmd);
    // }
    // server.p2p_seldb = dictid;

    /* Write the command to every replica. */
    listRewind(replicas, &li);
    while ((ln = listNext(&li)))
    {
        client *replica = ln->value;

        replica->flags |= CLIENT_REPLICA_MESSAGE;

        /* Don't feed replica that are still waiting for BGSAVE to start
        if (replica->replstate == SLAVE_STATE_WAIT_BGSAVE_START) continue;*/

        /* Add the multi bulk length. */
        addReplyArrayLen(replica, argc);

        /* Finally any additional argument that was not stored inside the
         * static buffer if any (from j to argc). */
        for (j = 0; j < argc; j++)
            addReplyBulk(replica, argv[j]);

//        if (selected)
//            addReply(replica, shared.exec_cmd);

        replica->flags &= ~CLIENT_REPLICA_MESSAGE;
    }
}
