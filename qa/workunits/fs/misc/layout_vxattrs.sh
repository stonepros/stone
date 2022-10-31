#!/usr/bin/env bash

set -ex

# detect data pool
datapool=
dir=.
while true ; do
    echo $dir
    datapool=$(getfattr -n stone.dir.layout.pool $dir --only-values) && break
    dir=$dir/..
done

# file
rm -f file file2
touch file file2

getfattr -n stone.file.layout file
getfattr -n stone.file.layout file | grep -q object_size=
getfattr -n stone.file.layout file | grep -q stripe_count=
getfattr -n stone.file.layout file | grep -q stripe_unit=
getfattr -n stone.file.layout file | grep -q pool=
getfattr -n stone.file.layout.pool file
getfattr -n stone.file.layout.pool_namespace file
getfattr -n stone.file.layout.stripe_unit file
getfattr -n stone.file.layout.stripe_count file
getfattr -n stone.file.layout.object_size file

getfattr -n stone.file.layout.bogus file   2>&1 | grep -q 'No such attribute'
getfattr -n stone.dir.layout file    2>&1 | grep -q 'No such attribute'

setfattr -n stone.file.layout.stripe_unit -v 1048576 file2
setfattr -n stone.file.layout.stripe_count -v 8 file2
setfattr -n stone.file.layout.object_size -v 10485760 file2

setfattr -n stone.file.layout.pool -v $datapool file2
getfattr -n stone.file.layout.pool file2 | grep -q $datapool
setfattr -n stone.file.layout.pool_namespace -v foons file2
getfattr -n stone.file.layout.pool_namespace file2 | grep -q foons
setfattr -x stone.file.layout.pool_namespace file2
getfattr -n stone.file.layout.pool_namespace file2 | grep -q -v foons

getfattr -n stone.file.layout.stripe_unit file2 | grep -q 1048576
getfattr -n stone.file.layout.stripe_count file2 | grep -q 8
getfattr -n stone.file.layout.object_size file2 | grep -q 10485760

setfattr -n stone.file.layout -v "stripe_unit=4194304 stripe_count=16 object_size=41943040 pool=$datapool pool_namespace=foons" file2
getfattr -n stone.file.layout.stripe_unit file2 | grep -q 4194304
getfattr -n stone.file.layout.stripe_count file2 | grep -q 16
getfattr -n stone.file.layout.object_size file2 | grep -q 41943040
getfattr -n stone.file.layout.pool file2 | grep -q $datapool
getfattr -n stone.file.layout.pool_namespace file2 | grep -q foons

setfattr -n stone.file.layout -v "stripe_unit=1048576" file2
getfattr -n stone.file.layout.stripe_unit file2 | grep -q 1048576
getfattr -n stone.file.layout.stripe_count file2 | grep -q 16
getfattr -n stone.file.layout.object_size file2 | grep -q 41943040
getfattr -n stone.file.layout.pool file2 | grep -q $datapool
getfattr -n stone.file.layout.pool_namespace file2 | grep -q foons

setfattr -n stone.file.layout -v "stripe_unit=2097152 stripe_count=4 object_size=2097152 pool=$datapool pool_namespace=barns" file2
getfattr -n stone.file.layout.stripe_unit file2 | grep -q 2097152
getfattr -n stone.file.layout.stripe_count file2 | grep -q 4
getfattr -n stone.file.layout.object_size file2 | grep -q 2097152
getfattr -n stone.file.layout.pool file2 | grep -q $datapool
getfattr -n stone.file.layout.pool_namespace file2 | grep -q barns

# dir
rm -f dir/file || true
rmdir dir || true
mkdir -p dir

getfattr -d -m - dir | grep -q stone.dir.layout       && exit 1 || true
getfattr -d -m - dir | grep -q stone.file.layout      && exit 1 || true
getfattr -n stone.dir.layout dir                      && exit 1 || true

setfattr -n stone.dir.layout.stripe_unit -v 1048576 dir
setfattr -n stone.dir.layout.stripe_count -v 8 dir
setfattr -n stone.dir.layout.object_size -v 10485760 dir
setfattr -n stone.dir.layout.pool -v $datapool dir
setfattr -n stone.dir.layout.pool_namespace -v dirns dir

getfattr -n stone.dir.layout dir
getfattr -n stone.dir.layout dir | grep -q object_size=10485760
getfattr -n stone.dir.layout dir | grep -q stripe_count=8
getfattr -n stone.dir.layout dir | grep -q stripe_unit=1048576
getfattr -n stone.dir.layout dir | grep -q pool=$datapool
getfattr -n stone.dir.layout dir | grep -q pool_namespace=dirns
getfattr -n stone.dir.layout.pool dir | grep -q $datapool
getfattr -n stone.dir.layout.stripe_unit dir | grep -q 1048576
getfattr -n stone.dir.layout.stripe_count dir | grep -q 8
getfattr -n stone.dir.layout.object_size dir | grep -q 10485760
getfattr -n stone.dir.layout.pool_namespace dir | grep -q dirns


setfattr -n stone.file.layout -v "stripe_count=16" file2
getfattr -n stone.file.layout.stripe_count file2 | grep -q 16
setfattr -n stone.file.layout -v "object_size=10485760 stripe_count=8 stripe_unit=1048576 pool=$datapool pool_namespace=dirns" file2
getfattr -n stone.file.layout.stripe_count file2 | grep -q 8

touch dir/file
getfattr -n stone.file.layout.pool dir/file | grep -q $datapool
getfattr -n stone.file.layout.stripe_unit dir/file | grep -q 1048576
getfattr -n stone.file.layout.stripe_count dir/file | grep -q 8
getfattr -n stone.file.layout.object_size dir/file | grep -q 10485760
getfattr -n stone.file.layout.pool_namespace dir/file | grep -q dirns

setfattr -x stone.dir.layout.pool_namespace dir
getfattr -n stone.dir.layout dir | grep -q -v pool_namespace=dirns

setfattr -x stone.dir.layout dir
getfattr -n stone.dir.layout dir     2>&1 | grep -q 'No such attribute'

echo OK

