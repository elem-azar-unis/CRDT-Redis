#!/bin/bash

./shutdown.sh
./clean.sh
./server.sh
./construct_replication.sh
./client.sh 6379
