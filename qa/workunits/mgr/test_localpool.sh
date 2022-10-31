#!/bin/sh -ex

stone config set mgr mgr/localpool/subtree host
stone config set mgr mgr/localpool/failure_domain osd
stone mgr module enable localpool

while ! stone osd pool ls | grep '^by-host-'
do
    sleep 5
done

stone mgr module disable localpool
for p in `stone osd pool ls | grep '^by-host-'`
do
    stone osd pool rm $p $p --yes-i-really-really-mean-it
done

stone config rm mgr mgr/localpool/subtree
stone config rm mgr mgr/localpool/failure_domain

echo OK
