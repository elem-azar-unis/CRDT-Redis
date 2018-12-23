#!/bin/bash

if [ $# == 0 ]
then
    rm -rf *.rdb *.log
else
    ports=($*)
    for port in ${ports[*]}
    do
        rm -rf ${port}.rdb ${port}.log
        echo "remove ${port} rdb and log files."
    done
fi

rm -rf ozlog rzlog
