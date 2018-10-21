#!/bin/bash

if [ $# == 0 ]
then
    ./server.sh
    ./construct_replication.sh
    ./client.sh 6379
elif [ $1 == "end" ]
then
    ./shutdown.sh
    ./clean.sh
elif [ $1 == "restart" ]
then
    ./shutdown.sh
    ./clean.sh
    ./server.sh
    ./construct_replication.sh
    ./client.sh 6379
elif [ $1 == "single" ]
then
    ./server.sh 6379
    ./client.sh 6379
fi
