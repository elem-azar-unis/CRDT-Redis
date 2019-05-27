Here we show how to test our 


本项目基于 Redis (4.0.8) 进行拓展。这里包含四个文件夹：

* *redis-4.0.8* ：更改后的项目文件夹
* *redis_test* ：测试用配置文件和脚本，实验环境搭建
* *记录* ：阶段完成后的总结
* *bench*：实验命令生成与测试端代码

## 部署与测试

    
## 阶段性进展

|	No.	|	名称		|	概要	|	
| ------------- | --------------------- | --------------------- |
| 1		| [P2P 拓展](./p2p.md)		|	完成了对 redis 的 P2P 复制拓展，建立起简单的 P2P 连接		| 
| 2		| [逻辑时钟实现](./vector_clock.md)	|	实现了逻辑时钟，有简单的操作以及命令，以及其持久化支持	| 
| 3		| [add-win RPQ 实现](./add-win%20RPQ.md)	|	依照算法设计，实现了 add-win RPQ (ORI RPQ)	| 
| 4 	| [remove-win RPQ 实现](./remove-win%20RPQ.md)	|	依照算法设计，实现了 remove-win RPQ	|