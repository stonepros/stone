#!/usr/bin/env bash
#
# Copyright (C) 2014 Cloudwatt <libre.licensing@cloudwatt.com>
# Copyright (C) 2014, 2015 Red Hat <contact@redhat.com>
#
# Author: Loic Dachary <loic@dachary.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library Public License for more details.
#

source $STONE_ROOT/qa/standalone/stone-helpers.sh

function run() {
    local dir=$1
    shift

    export STONE_MON="127.0.0.1:7106" # git grep '\<7106\>' : there must be only one
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "
    STONE_ARGS+="--mon-host=$STONE_MON "
    STONE_ARGS+="--debug-bluestore 20 "

    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
        $func $dir || return 1
        teardown $dir || return 1
    done
}

function TEST_bench() {
    local dir=$1

    run_mon $dir a || return 1
    run_mgr $dir x || return 1
    run_osd $dir 0 || return 1

    local osd_bench_small_size_max_iops=$(STONE_ARGS='' stone-conf \
        --show-config-value osd_bench_small_size_max_iops)
    local osd_bench_large_size_max_throughput=$(STONE_ARGS='' stone-conf \
        --show-config-value osd_bench_large_size_max_throughput)
    local osd_bench_max_block_size=$(STONE_ARGS='' stone-conf \
        --show-config-value osd_bench_max_block_size)
    local osd_bench_duration=$(STONE_ARGS='' stone-conf \
        --show-config-value osd_bench_duration)

    #
    # block size too high
    #
    expect_failure $dir osd_bench_max_block_size \
        stone tell osd.0 bench 1024 $((osd_bench_max_block_size + 1)) || return 1

    #
    # count too high for small (< 1MB) block sizes
    #
    local bsize=1024
    local max_count=$(($bsize * $osd_bench_duration * $osd_bench_small_size_max_iops))
    expect_failure $dir bench_small_size_max_iops \
        stone tell osd.0 bench $(($max_count + 1)) $bsize || return 1

    #
    # count too high for large (>= 1MB) block sizes
    #
    local bsize=$((1024 * 1024 + 1))
    local max_count=$(($osd_bench_large_size_max_throughput * $osd_bench_duration))
    expect_failure $dir osd_bench_large_size_max_throughput \
        stone tell osd.0 bench $(($max_count + 1)) $bsize || return 1

    #
    # default values should work
    #
    stone tell osd.0 bench || return 1

    #
    # test object_size < block_size
    stone tell osd.0 bench 10 14456 4444 3
    #

    #
    # test object_size < block_size & object_size = 0(default value)
    #
    stone tell osd.0 bench 1 14456
}

main osd-bench "$@"

# Local Variables:
# compile-command: "cd ../.. ; make -j4 && test/osd/osd-bench.sh"
# End:
