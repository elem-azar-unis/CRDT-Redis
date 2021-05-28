# Conflict-Free Replicated Data Types Based on Redis

Several Conflict-Free Replicated Data Types (CRDTs) implemented based on Redis(6.0.5).

Things we do for such implementation:

* Enable Redis to replicate in P2P mode.
* Implement the CRDT framework.
* Implement specific CRDTs according to their algorithms.

The CRDTs currently implemented:

* Replicated Priority Queue (RPQ)
  * [Add-Win RPQ](document/add-win-crpq.pdf)
  * [Remove-Win RPQ](document/rwf-tr.pdf)
  * [RWF-RPQ](document/rwf-tr.pdf)
* List
  * [Remove-Win List](document/rwf-tr.pdf)
  * [RWF-List](document/rwf-tr.pdf)

For more details of our implementation, please read the linked article of the CRDTs above. Also you can read the *Performance measurements* section of [the technical report](https://arxiv.org/abs/1905.01403). Note that the **Remove-Win RPQ** in this technical report corresponds to the **RWF-RPQ** here.

## Build

Our modified Redis is in folder *redis-6.0.5*. Please build it in the default mode:

```bash
cd redis-6.0.5
make
sudo make install
```

## Test

In the folder *experiment/redis_test* there are scripts for testing our modified Redis and RPQ implementations. By default, the test will start 5 Redis server instances on the local machine, listening on port 6379, 6380, 6381, 6382 and 6383. You may choose to start some of these server instances by using parameters when you run the scripts.

Here we focus on 5 scripts:

* **server.sh [parameters]** Start the 5 Redis instances, or the Redis instances specified by parameters.
* **construct_replication.sh [parameters]** Construct P2P replication among the 5 Redis instances, or the Redis instances specified by parameters.
* **client.sh <server_port>** Start a client to connect to the Redis server listening on the specified port.
* **shutdown.sh [parameters]** Close the 5 Redis instances, or the Redis instances specified by parameters.
* **clean.sh [parameters]** Remove the database files (.rdb files) and log files (.log files) of the 5 Redis instances, or the Redis instances specified by parameters.

Here we show an example to test our implementation using all the 5 Redis server instances.

Firstly go to the *experiment/redis_test*, **start** the Redis server instances and **construct P2P replication** among them.

```bash
cd experiment/redis_test
./server.sh
./construct_replication.sh
```

Now you can start redis clients to connect to Redis servers and do RPQ read/write operations. Here shows **start a client** to connect one Redis server.

```bash
./client.sh <server_port>
```

When you've finished testing, you may **close** the Redis server instances.

```bash
./shutdown.sh
```

Finally, you can **remove** the Redis database files (.rdb files) and log files (.log files).

```bash
./clean.sh
```

To further redo the experiment of our work our please check [here](experiment/README.md).

## CRDT Operations

Different implementations of the same type of CRDT offer the same operations for users. We use **[type][op]** to name our CRDT operations. The operations name contains of an implementation type prefix and an operation type suffix. For example, the name of operation of RWF-RPQ to add one element is **rwfzadd**, where **rwf** is the implementation type prefix and **zadd** is the operation type suffix.

### RPQ operations

The **[type]** prefix of different RPQs are:

* Add-Win RPQ : **o**
* Remove-Win RPQ : **r**
* RWF-RPQ : **rwf**

The operations of RPQs are:

* **[type]zadd Q E V** : Add a new element *E* into the priority queue *Q* with initial value *V*.
* **[type]zincrby Q E I** : Add the increment *I* to the value of element *E* in the priority queue *Q*.
* **[type]zrem Q E** : Remove element *E* from the priority queue *Q*.
* **[type]zscore Q E** : Read the value of element *E* from the priority queue *Q*.
* **[type]zmax Q** : Read the element with the largest value in the priority queue *Q*. Returns the element and its value.

### List operations

The **[type]** prefix of different Lists are:

* Remove-Win List : **r**
* RWF-List : **rwf**

The operations of Lists are:

* **[type]linsert L prev id content font size color property** : Add a new element *id* after *prev* into the list *L*, with the content *content*, and the initial properties *font* *size* *color* and *property*(bitmap that encodes bold, italic and underline). If *prev* is "null", it means insert the element at the beginning of the list. If *prev* is "readd", it means to re-add the element that is previously added and then removed, and restore its position.
* **[type]lupdate L id type value** : Update the *type* property with *value* for element *id* in list *L*.
* **[type]lrem L id** : Remove element *id* from the list *L*.
