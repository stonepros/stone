#! /bin/sh -x

#
# Create a bunch of pools in parallel
# This test isn't very smart -- run it from your src dir.
#

set -e

STONE_NUM_MON=1 STONE_NUM_MDS=1 STONE_NUM_OSD=$2 ./vstart.sh -n -d

num_osd=$2
maxosd=$((num_osd-1))
for osd_num in `seq 0 $maxosd`; do
    ./stone osd tell $osd_num start_profiler
done

for i in `seq 0 $1`; do
    for j in `seq 0 9`; do
	poolnum=$((i*10+j))
	poolname="pool$poolnum"
	./stone osd pool create $poolname 8 &
    done
    wait
done
