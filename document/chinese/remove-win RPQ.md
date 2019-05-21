阶段成果
---------
依照算法设计，实现了 remove-win RPQ 

remove-win RPQ 有 5 条命令，其格式及含义为：

* **rzadd RPQ element initial_value :** 向 *RPQ* 新增 *element*，初始值为 *initial_value*
* **rzincby RPQ element increment :** 使 *RPQ* 中的 *element* 值增加 *increment*
* **rzrem RPQ element :** 删除 *RPQ* 中的 *element*
* **rzscore RPQ element :** 读取 *RPQ* 中的 *element* 的值
* **rzmax RPQ :** 读取 *RPQ* 中最大值元素及其值

上面的命令支持 p2p 复制，并维持最终一致性，算法见论文

remove-win RPQ 设计细节
---------- 
- 结构设计：一个 remove-win RPQ 在实际存储为两部分
    - 存储存在的元素的值顺序的有序集合，使用 redis 原有的 sorted set 底层实现。对外表现的有序集合。
    - 存储元素信息的哈希表使用 redis 原有的 hash set 底层实现。哈希表在有序集合的名字基础上加一个后缀存储，对外不可见。由于哈希表的设计中值只能是字符串，因此将自定数据结构指针值当作字符串存入其中。
- 服务于该 RPQ 的结构体及其操作
    * *vc*，向量时钟，使用之前做好的实现。主要使用到其创建、删除、与字符串互相转码，比较，新增判断完全相等函数，判断 causally ready 函数。
    * *rze*，元素信息，类似 add-win 中 oze。有当前本地时间戳，先天值，后天值，添加编号，等待操作队列。函数：创建。
    * *ucmd*，未就绪的操作。记录有操作类型，RPQ 名称，元素名称，操作值，时间戳。函数：创建、删除。
- 实用函数/宏：
    * *LOOKUP*，宏，判断元素是否存在。
    * *SCORE*，宏，得到元素优先级值。
    * *insertCheck*，*increaseCheck*，*removeCheck*，三种操作的准入条件判断。
    * *readyCheck*，添加、增量写操作的 preCondition 判断。
    * *insertFunc*，*increaseFunc*，添加、增量写操作的具体执行函数，单独写出来便于 notify 调用。
    * *notifyLoop*，notify 列表中所有的暂存操作并尝试执行。
    * *now*，仅关联删除操作的时间戳，新时间戳获得函数。
    * *getInnerHT*，获得 RPQ 的内部哈希表，不存在则依据参数是否创建。
    * *rzeHTGet*，获得元素存储数据结构，不存在则依据参数是否创建。
    * *checkArgcAndZsetType*，atSource 操作检查，操作数、获取的数据类型是否正确。
    * *getZsetOrCreate*，查找有序集合，没有则创建。
- 写操作运行逻辑
    1. atSource 阶段，检查参数个数，参数类型，集合类型，准备好新的参数并准备好新参数向量
    2. downStream 阶段，解析出所有的参数
        * insert 操作和 increase 操作判断能否执行，能执行则判断准入、具体执行；不能则将操作记录进 ucmd 列表
        * remove 操作判断准入，能执行则执行操作后，notify 列表中的所有操作进行执行

代码修改介绍
----------- 
* 新文件 *t_rzset.c*，remove-win RPQ 实现于此，具体见上面的细节介绍
* *server.h* & *server.c*
    * 添加 remove-win RPQ 的命令函数，加入函数表
    * 将 add-win RPQ 中几个实用函数声明放到顶层，方便调用
* *vector_clock.h* & *vector_clock.c*
    * clock 添加判断相等函数
    * causally ready 函数，方便 causal delivery（本次没有使用）