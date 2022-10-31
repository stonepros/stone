#!/bin/sh

TEST_POOL=rbd

./stop.sh
STONE_NUM_OSD=3 ./vstart.sh -d -n -x -o 'osd min pg log entries = 5'
./rados -p $TEST_POOL bench 15 write -b 4096
./stone osd out 0
 ./init-stone stop osd.0
 ./stone osd down 0
./rados -p $TEST_POOL bench 600 write -b 4096
