#!/usr/bin/env bash
set -x

#
# Runs the synthetic client
#

# Includes
source "`dirname $0`/test_common.sh"

# Functions
setup() {
        export STONE_NUM_OSD=$1

        # Start stone
        ./stop.sh

        # set recovery start to a really long time to ensure that we don't start recovery
        ./vstart.sh -d -n -o 'osd recovery delay start = 10000
osd max scrubs = 0' || die "vstart failed"
}

csyn_simple1_impl() {
  ./stone-syn -c ./stone.conf --syn writefile 100 1000 --syn writefile 100 1000 || die "csyn failed"
}

csyn_simple1() {
  setup 2
  csyn_simple1_impl
}

run() {
        csyn_simple1 || die "test failed"
}

$@
