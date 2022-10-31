#!/usr/bin/env bash

source $STONE_ROOT/qa/standalone/stone-helpers.sh

mon_port=$(get_unused_port)

function run() {
    local dir=$1
    shift

    export STONE_MON="127.0.0.1:$mon_port"
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

function TEST_bad_inc_map() {
    local dir=$1

    run_mon $dir a
    run_mgr $dir x
    run_osd $dir 0
    run_osd $dir 1
    run_osd $dir 2

    stone config set osd.2 osd_inject_bad_map_crc_probability 1

    # osd map churn
    create_pool foo 8
    stone osd pool set foo min_size 1
    stone osd pool set foo min_size 2

    sleep 5

    # make sure all the OSDs are still up
    TIMEOUT=10 wait_for_osd up 0
    TIMEOUT=10 wait_for_osd up 1
    TIMEOUT=10 wait_for_osd up 2

    # check for the signature in the log
    grep "injecting map crc failure" $dir/osd.2.log || return 1
    grep "bailing because last" $dir/osd.2.log || return 1

    echo success

    delete_pool foo
    kill_daemons $dir || return 1
}

main bad-inc-map "$@"

# Local Variables:
# compile-command: "make -j4 && ../qa/run-standalone.sh bad-inc-map.sh"
# End:
