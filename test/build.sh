#!/bin/bash

#1. test net_base
#g++ test_net_base.cpp -o test_net_base

#2. test circular buffer
#g++ test_pod_circularbuffer.cpp -o test_pod_circularbuffer

#3. test tcp server
g++ test_tcpserver.cpp ../tcpserver.cpp ../log.c -o test_tcpserver --std=c++0x -I../ -I/usr/local/include  -L/usr/local/lib -lpthread -luv -lssl -lcrypto
