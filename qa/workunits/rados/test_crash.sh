#!/bin/sh

set -x

# run on a single-node three-OSD cluster

sudo killall -ABRT stone-osd
sleep 5

# kill caused coredumps; find them and delete them, carefully, so as
# not to disturb other coredumps, or else teuthology will see them
# and assume test failure.  sudos are because the core files are
# root/600
for f in $(find $TESTDIR/archive/coredump -type f); do
	gdb_output=$(echo "quit" | sudo gdb /usr/bin/stone-osd $f)
	if expr match "$gdb_output" ".*generated.*stone-osd.*" && \
	   ( \

	   	expr match "$gdb_output" ".*terminated.*signal 6.*" || \
	   	expr match "$gdb_output" ".*terminated.*signal SIGABRT.*" \
	   )
	then
		sudo rm $f
	fi
done

# let daemon find crashdumps on startup
sudo systemctl restart stone-crash
sleep 30

# must be 3 crashdumps registered and moved to crash/posted
[ $(stone crash ls | wc -l) = 4 ]  || exit 1   # 4 here bc of the table header
[ $(sudo find /var/lib/stonepros/crash/posted/ -name meta | wc -l) = 3 ] || exit 1

# there should be a health warning
stone health detail | grep RECENT_CRASH || exit 1
stone crash archive-all
sleep 30
stone health detail | grep -c RECENT_CRASH | grep 0     # should be gone!
