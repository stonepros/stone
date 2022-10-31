#!/usr/bin/env bash

source $STONE_ROOT/qa/standalone/stone-helpers.sh

set -e

function run() {
    local dir=$1
    shift

    export STONE_MON="127.0.0.1:$(get_unused_port)"
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "
    STONE_ARGS+="--mon-host=$STONE_MON "
    set -e

    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
	$func $dir || return 1
        teardown $dir || return 1
    done
}

function TEST_safe_to_destroy() {
    local dir=$1

    run_mon $dir a
    run_mgr $dir x
    run_osd $dir 0
    run_osd $dir 1
    run_osd $dir 2
    run_osd $dir 3

    flush_pg_stats

    stone osd safe-to-destroy 0
    stone osd safe-to-destroy 1
    stone osd safe-to-destroy 2
    stone osd safe-to-destroy 3

    stone osd pool create foo 128
    sleep 2
    flush_pg_stats
    wait_for_clean

    expect_failure $dir 'pgs currently' stone osd safe-to-destroy 0
    expect_failure $dir 'pgs currently' stone osd safe-to-destroy 1
    expect_failure $dir 'pgs currently' stone osd safe-to-destroy 2
    expect_failure $dir 'pgs currently' stone osd safe-to-destroy 3

    stone osd out 0
    sleep 2
    flush_pg_stats
    wait_for_clean

    stone osd safe-to-destroy 0

    # even osds without osd_stat are ok if all pgs are active+clean
    id=`stone osd create`
    stone osd safe-to-destroy $id
}

function TEST_ok_to_stop() {
    local dir=$1

    run_mon $dir a
    run_mgr $dir x
    run_osd $dir 0
    run_osd $dir 1
    run_osd $dir 2
    run_osd $dir 3

    stone osd pool create foo 128
    stone osd pool set foo size 3
    stone osd pool set foo min_size 2
    sleep 1
    flush_pg_stats
    wait_for_clean

    stone osd ok-to-stop 0
    stone osd ok-to-stop 1
    stone osd ok-to-stop 2
    stone osd ok-to-stop 3
    expect_failure $dir bad_become_inactive stone osd ok-to-stop 0 1

    stone osd pool set foo min_size 1
    sleep 1
    flush_pg_stats
    wait_for_clean
    stone osd ok-to-stop 0 1
    stone osd ok-to-stop 1 2
    stone osd ok-to-stop 2 3
    stone osd ok-to-stop 3 4
    expect_failure $dir bad_become_inactive stone osd ok-to-stop 0 1 2
    expect_failure $dir bad_become_inactive stone osd ok-to-stop 0 1 2 3
}

main safe-to-destroy "$@"
