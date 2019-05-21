阶段成果
---------
依照算法设计，实现了 add-win RPQ (ORI RPQ)

add-win RPQ 有 5 条命令，其格式及含义为：

* **ozadd RPQ element initial_value :** 向 *RPQ* 新增 *element*，初始值为 *initial_value*
* **ozincby RPQ element increment :** 使 *RPQ* 中的 *element* 值增加 *increment*
* **ozrem RPQ element :** 删除 *RPQ* 中的 *element*
* **ozscore RPQ element :** 读取 *RPQ* 中的 *element* 的值
* **ozmax RPQ :** 读取 *RPQ* 中最大值元素及其值

上面的命令支持 p2p 复制，并维持最终一致性，算法见论文

add-win RPQ 设计细节
---------- 
- 结构设计：一个 add-win RPQ 在实际存储为两部分
    - 存储存在的元素的值顺序的有序集合，使用 redis 原有的 sorted set 底层实现。对外表现的有序集合。
    - 存储元素信息的哈希表使用 redis 原有的 hash set 底层实现。哈希表在有序集合的名字基础上加一个后缀存储，对外不可见。由于哈希表的设计中值只能是字符串，因此将自定数据结构指针值当作字符串存入其中。
- 服务于该 RPQ 的新结构体及其操作
    * *ct*，最简单的 `<PID,num>` 格式的 clock。有创建、比较、复制、与字符串 sds 相互转换函数。注：此处的 clock 使用是特殊的，不跟据他者更新，仅自增。
    * *ase*，添加集元素，有 clock，先天后天值，改变量。有创建函数。
    * *oze*，元素信息，有当前自然数，最优先天值，最优后天值，添加集，删除集。函数：添加集扩充，添加集查询，删除集查询，计算值。
- 实用函数/宏：
    * *LOOKUP*，宏，判断元素是否存在。
    * *update_innate_value*，更新先天值，查询不到就创建，并更新最优先天值。
    * *update_acquired_value*，更新后天值，查询不到就创建，并更新最优后天值。
    * *remove_tag*，删除一个添加集元素，牵扯到最优先天/后天值则标注，不重排。
    * *resort*，重新选出最优先天/后天值。
    * *getInnerHT*，获得 RPQ 的内部哈希表，不存在则依据参数是否创建。
    * *ozeHTGet*，获得元素存储数据结构，不存在则依据参数是否创建。
    * *checkArgcAndZsetType*，atSource 操作检查，操作数、获取的数据类型是否正确。
    * *getZsetOrCreate*，查找有序集合，没有则创建。

代码修改介绍
----------- 
* 新文件 *t_ozset.c*，add-win RPQ 实现于此，具体见上面的细节介绍
* *server.h* & *server.c*
    * 添加 add-win RPQ 的命令函数，加入函数表
    * 添加两条新的通用消息：元素已存在，元素不存在，并初始化
    * 添加几个宏，方便 rargv 的准备
* *vector_clock.h* & *vector_clock.c*
    * clock 添加判断相等函数
    * 实用的宏，获得本地 ID，创建新时间戳等
    * 使用 sds 自带函数重写 vc 与 sds 相互转换函数
    * 利用上面 rargv 准备相关的新宏重写了 atSource 中参数准备部分