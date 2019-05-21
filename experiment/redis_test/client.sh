#!/bin/bash

REDIS_PORT=$1

redis-cli -h 127.0.0.1 -p ${REDIS_PORT}
