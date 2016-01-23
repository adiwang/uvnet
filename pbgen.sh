#!/bin/sh

SRC_DIR=./protocol
DST_DIR=./pb

rm -rf $DST_DIR/*.pb.h $DST_DIR/*.pb.cc

#export LD_LIBRARY_PATH=./protobuf/lib
protoc -I$SRC_DIR --cpp_out=$DST_DIR $SRC_DIR/*.proto
