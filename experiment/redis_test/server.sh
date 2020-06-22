#!/bin/bash

if [ $# == 0 ]
then
    ports=(6379 6380 6381 6382 6383)
else
    ports=($*)
fi

for port in ${ports[*]}
do
    redis-server ../../redis-4.0.8/redis.conf --protected-mode no --daemonize yes --loglevel debug --port ${port} --logfile ${port}.log --dbfilename ${port}.rdb --pidfile /var/run/redis_${port}.pid
    echo "server ${port} started."
done
