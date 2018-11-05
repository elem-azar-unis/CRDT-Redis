#!/bin/bash

if [ $# == 0 ]
then
    ports=(6379 6380 6381 6382 6383)
else
    ports=($*)
fi

for port in ${ports[*]}
do
    rm -rf ${port}.rdb ${port}.log
    echo "remove ${port} rdb and log files."
done

rm -rf ozlog rzlog
