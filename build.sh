#!/bin/sh

export LD_LIBRARY_PATH=./protobuf/lib
g++ test_netmessage.cpp pb/netmessage.pb.cc -o test_netmessage -I. -L/usr/local/lib -lpthread -lprotobuf