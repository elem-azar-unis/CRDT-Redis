# CRPQ Experiments

Here are the tests and the experiments we do on our CPRQ implementations. There are 3 folders here:

* *redis_test* : The testing bash scripts and server configuration files. Also used in the experiment.
  * **.conf* : The configuration files for our modified Redis servers.
  * **.sh* : Bash scripts for starting and testing on local machine.
  * *test.py* : Local test we do to test the correctness of our CRPQ implementations.
  * *connection.py* : Construct the server part of experiment framework, close it and clean it when the experiment is finished. It use ssh to control the server VMs to start their Redis instances, construct replication among them, and add networking delay between them. When the experiment is finished, close all the server and remove their .rdb and .log files.
* *bench* : The experiment code. Constructs the experiment, generates and runs operations, get logs and results. 
  * *constants.h* : The constants of our experiment.
  * *generator.h* : The generator of CRPQ operations. Including write operations with deliberate concurrent conflicts.
  * *queue_log.h*, *queue_log.cpp* : Log the execution of the write operations and the results of the read operations during the experiment. Write the results to the *result* folder at the end of the experiment.
  * *main.cpp* : The entry of the experiment. It calls *redis_test/connection.py* to construct the experiment environment. Then starts multiple Redis client threads for each Redis server. It uses the generator to generate workload to run at the client threads. Finally it lets the log to write the results.
* *result* : The results and the data analysis.
  * *result.py* : Compute the statistics of the result data and draw result figures.
  * *result.zip* : The result data and figures of our experiment packed in a zip file.

## Experiment Framework

<div align=center><img src="exp-crop.png" width="65%" height="65%"></div>

We use VMware to run 4 virtual machines. VM 1-3 run Ubuntu server 16.04.5 LTS. VM4 runs Ubuntu desktop 16.04.5 LTS. Each of VM 1-3 is deployed with Redis server instances and simulates a data center. VM4 runs all the clients. 

The ips of VM 1-3 are *192.168.188.135*, *192.168.188.136*, *192.168.188.137*. You can see the ip settings in the beginning of *bench/main.cpp* and *redis_test/connection.py*.

The entry of the experiment is *bench/main.cpp*. It will do 30 rounds of experiment for each experiment settings. Each round it will call *redis_test/connection.py* to start new Redis server instances in all the server VMs and set the networking delays. The results will be written in the *results* folder in different subfolders according to different experiment settings and rounds.

## Perform the Experiment

To perform our CRPQ experiments, setup the experiment as shown above. You need to modify the ip settings at the start of *bench/main.cpp* and *redis_test/connection.py* if you want to use other ips for servers. Then start the experiment:

    cd bench
    make run

After it finished you will get the data in the *result* folder. 
You may then draw figures by (python 3.6 required):

    cd ../result
    python result.py

Note that there may be some bugged results that are apparently invalid. You may find them by using the according find function in *result/result.py*, and then fix them by use the according fix functions in *bench/main.cpp* accordingly.

For more detail of our implementation, please read the *Performance measurements* section of [the technical report](https://arxiv.org/abs/1905.01403).
