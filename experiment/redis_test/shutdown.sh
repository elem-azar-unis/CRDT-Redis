#!/bin/bash

if [ $# == 0 ]
then
    ports=(6379 6380 6381 6382 6383)
else
    ports=($*)
fi

for port in ${ports[*]}
do
    redis-cli -h 127.0.0.1 -p ${port} SHUTDOWN NOSAVE
    echo "server ${port} shutdown."
done
