#!/bin/sh -ex

TEST_POOL=ecpool
TEST_IMAGE=test1
PGS=12

stone osd pool create $TEST_POOL $PGS $PGS erasure
stone osd pool application enable $TEST_POOL rbd
stone osd pool set $TEST_POOL allow_ec_overwrites true
rbd --data-pool $TEST_POOL create --size 1024G $TEST_IMAGE
rbd bench \
    --io-type write \
    --io-size 4096 \
    --io-pattern=rand \
    --io-total 100M \
    $TEST_IMAGE

echo "OK"
