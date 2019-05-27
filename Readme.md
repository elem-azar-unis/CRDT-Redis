# Conflict-Free Rpelicated Priority Queues Based on Redis

[**Add-Win CRPQ**](document/add-win-crpq.pdf) and [**Remove-Win CRPQ**](document/rmv-wim-crpq.bib) implementations based on Redis (4.0.8). 

Things we do for such implementation:

* Enable Redis to replicate in P2P mode
* Implement the CRDT framework
* Implement Add-Win CRPQ and Remove-Win CRPQ

For more detail of our implementation, please read the *Performance measurements* section of [the article](document/rmv-wim-crpq.bib).


## Build and Test

Our modified Redis is in folder redis-4.0.8. Please build it in the default mode:

    cd redis-4.0.8
    sudo make install

文件夹 *redis_test* 中包含测试用配置文件和脚本，测试会在本地启动 5 个 Redis 服务器，监听端口号为：6379, 6380, 6381, 6382, 6383，下面脚本参数从这几个端口号中选一个到多个

有 5 个脚本文件：

* server.sh [+参数] ：启动这五个服务器。有参数则启动参数指定服务器
* construct_replication.sh [+参数] ：将五个服务器建立 P2P 复制。有参数则为指定服务器建立复制
* client.sh <端口号> ：启动客户端链接指定端口号服务器
* shutdown.sh [+参数] ：关闭五个服务器。有参数则关闭指定服务器
* clean.sh [+参数] ：清除五个服务器的数据库数据文件和日志文件。有参数则清除指定数据库的文件

之后，启动 Redis 服务器并建立 P2P 复制

    ./server.sh
    ./construct_replication.sh

此时可以用 redis 客户端链接服务器

    ./client.sh <端口号>

测试完毕后关闭 5 个 Redis 客户端

    ./shutdown.sh

最后，可以清除 Redis 数据库数据文件以及日志文件

    ./clean.sh

To further redo the experiment of our work our please check [here](experiment/Readme.md).