#!/bin/bash

if [ $# == 0 ]
then
    ports=(6379 6380 6381 6382 6383)
else
    ports=($*)
fi

redis-cli -h 127.0.0.1 -p ${ports[0]} REPLICATE ${#ports[*]} 0
for (( i=1;i<${#ports[*]};i++ ))
do
    repl[(i-1)*2]=127.0.0.1
    repl[(i-1)*2+1]=${ports[i-1]}
    redis-cli -h 127.0.0.1 -p ${ports[i]} REPLICATE ${#ports[*]} $i ${repl[*]}
    echo "server ${ports[i]} replicate to ${ports[*]:0:i}"
done
