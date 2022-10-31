#!/bin/sh -ex

p=`uuidgen`

# objects
stone osd pool create $p 12
stone osd pool set-quota $p max_objects 10
stone osd pool application enable $p rados

for f in `seq 1 10` ; do
 rados -p $p put obj$f /etc/passwd
done

sleep 30

rados -p $p put onemore /etc/passwd  &
pid=$!

stone osd pool set-quota $p max_objects 100
wait $pid 
[ $? -ne 0 ] && exit 1 || true

rados -p $p put twomore /etc/passwd

# bytes
stone osd pool set-quota $p max_bytes 100
sleep 30

rados -p $p put two /etc/passwd &
pid=$!

stone osd pool set-quota $p max_bytes 0
stone osd pool set-quota $p max_objects 0
wait $pid 
[ $? -ne 0 ] && exit 1 || true

rados -p $p put three /etc/passwd


#one pool being full does not block a different pool

pp=`uuidgen`

stone osd pool create $pp 12
stone osd pool application enable $pp rados

# set objects quota 
stone osd pool set-quota $pp max_objects 10
sleep 30

for f in `seq 1 10` ; do
 rados -p $pp put obj$f /etc/passwd
done

sleep 30

rados -p $p put threemore /etc/passwd 

stone osd pool set-quota $p max_bytes 0
stone osd pool set-quota $p max_objects 0

sleep 30
# done
stone osd pool delete $p $p --yes-i-really-really-mean-it
stone osd pool delete $pp $pp --yes-i-really-really-mean-it

echo OK

