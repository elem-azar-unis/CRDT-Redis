#!/bin/bash

if [ $# == 0 ]
then
    ports=(6379 6380 6381 6382 6383)
else
    ports=($*)
fi

for port in ${ports[*]}
do
    redis-server ../../redis-6.0.5/redis.conf --protected-mode no --daemonize yes --loglevel debug --port ${port} --logfile ${port}.log --dbfilename ${port}.rdb --pidfile /var/run/redis_${port}.pid --io-threads 2
    echo "server ${port} started."
done
