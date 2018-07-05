代码修改介绍
-----------
主要代码修改有：

* 新文件 *p2p.c*，其中实现了新的命令 REPLICATE 的处理函数（该函数加入了 server.c 中的命令处理函数表）、服务器连接函数、replica 服务器广播函数
* *server.h* 中 sharedObjectsStruct 新的共享消息 *replhandshake, *multi, *exec，并在 *server.c* 中初始化
* *server.h* 中结构体 RedisServer 新增列表 replicas，并在 *server.c* 中初始化
* *server.h* 中 client 结构体加入 为复制使用的参数 rargc, rargv
* 客户端新的标志 CLIENT_REPLICA 与 CLIENT_REPLICA_MESSAGE; 后者在 *networking.c* 中 prepareClientToWrite 函数控制使用
* *server.c* 中 call 函数加入参数准备，判断、调用 replica 广播函数
* *server.h* 中定义宏 REPLICATION_MODE, CRDT_BEGIN, CRDT_ATSOURCE, CRDT_DOWNSTREAM, CRDT_END，后四者成组出现用作在 *t_set.c* 中 saddCommand 中，作为 SADD 命令的 P2P 复制拓展
* 其他（应该有一些现在想不起来了的） 
