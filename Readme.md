# Conflict-Free Rpelicated Priority Queues Based on Redis

[**Add-Win CRPQ**](document/add-win-crpq.pdf) and [**Remove-Win CRPQ**](https://arxiv.org/abs/1905.01403) implementations based on Redis (4.0.8). 

Things we do for such implementation:

* Enable Redis to replicate in P2P mode.
* Implement the CRDT framework.
* Implement Add-Win CRPQ and Remove-Win CRPQ.

For more detail of our implementation, please read the *Performance measurements* section of [the technical report](https://arxiv.org/abs/1905.01403).


## Build

Our modified Redis is in folder *redis-4.0.8*. Please build it in the default mode:

    cd redis-4.0.8
    sudo make install

## Test

In the folder *experiment/redis_test* there are configuration files and scripts for testing our modified Redis and CRPQ implementations. By default the test will start 5 Redis server instances on the local machine, listening on port 6379, 6380, 6381, 6382 and 6383. You may choose to start some of these server instances by using parameters when you run the scripts.

Here we focus on 5 scripts:

* **server.sh [parameters]** Start the 5 Redis instances, or the Redis instances specified by parameters.
* **construct_replication.sh [parameters]** Construct P2P replication among the 5 Redis instances, or the Redis instances specified by parameters.
* **client.sh <server_port>** Start a client to connect to the Redis server listening on the specified port.
* **shutdown.sh [parameters]** Close the 5 Redis instances, or the Redis instances specified by parameters.
* **clean.sh [parameters]** Remove the database files (.rdb files) and log files (.log files) of the 5 Redis instances, or the Redis instances specified by parameters.


Here we show an example to test our implementation using all the 5 Redis server instances.

Firstly goto the *experiment/redis_test*, **start** the Redis server instances and **construct P2P replication** among them.

    cd experiment/redis_test
    ./server.sh
    ./construct_replication.sh

Now you can start redis clients to connect to Redis servers and do CRPQ read/write operations. Here shows **start a client** to connect one Redis server.

    ./client.sh <server_port>

When you've finished testing you may **close** the Redis server instances.

    ./shutdown.sh

Finally you can **remove** the Redis database files (.rdb files) and log files (.log files).

    ./clean.sh

To further redo the experiment of our work our please check [here](experiment/Readme.md).

## CRPQ operations 

There are 5 operations of **Add-Win CRPQ** implemented in our modified Redis:

* **ozadd Q E V** : Add a new element *E* into the priority queue *Q* with initial value *V*.
* **ozincby Q E I** : Add the increment *I* to the value of element *E* in the priority queue *Q*.
* **ozrem Q E** : Remove element *E* from the priority queue *Q*.
* **ozscore Q E** : Read the value of element *E* from the priority queue *Q*.
* **ozmax Q** : Read the element with largest value in the priority queue *Q*. Returns the element and its value.

There are 5 operations of **Remove-Win CRPQ** implemented in our modified Redis:

* **rzadd Q E V** : Add a new element *E* into the priority queue *Q* with initial value *V*.
* **rzincby Q E I** : Add the increment *I* to the value of element *E* in the priority queue *Q*.
* **rzrem Q E** : Remove element *E* from the priority queue *Q*.
* **rzscore Q E** : Read the value of element *E* from the priority queue *Q*.
* **rzmax Q** : Read the element with largest value in the priority queue *Q*. Returns the element and its value.