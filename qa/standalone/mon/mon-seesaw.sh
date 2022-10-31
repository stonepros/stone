#!/usr/bin/env bash

source $STONE_ROOT/qa/standalone/stone-helpers.sh

function run() {
    local dir=$1
    shift

    export STONE_MON_A="127.0.0.1:7139" # git grep '\<7139\>' : there must be only one
    export STONE_MON_B="127.0.0.1:7141" # git grep '\<7141\>' : there must be only one
    export STONE_MON_C="127.0.0.1:7142" # git grep '\<7142\>' : there must be only one
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "

    export BASE_STONE_ARGS=$STONE_ARGS
    STONE_ARGS+="--mon-host=$STONE_MON_A "

    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
        $func $dir || return 1
        teardown $dir || return 1
    done
}

function TEST_mon_seesaw() {
    local dir=$1

    setup $dir || return

    # start with 1 mon
    run_mon $dir aa --public-addr $STONE_MON_A || return 1
    run_mgr $dir x || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1

    wait_for_quorum 300 1 || return 1

    # add in a second
    run_mon $dir bb --public-addr $STONE_MON_B || return 1
    STONE_ARGS="$BASE_STONE_ARGS --mon-host=$STONE_MON_A,$STONE_MON_B"
    wait_for_quorum 300 2 || return 1

    # remove the first one
    stone mon rm aa || return 1
    STONE_ARGS="$BASE_STONE_ARGS --mon-host=$STONE_MON_B"
    sleep 5
    wait_for_quorum 300 1 || return 1

    # do some stuff that requires the osds be able to communicate with the
    # mons.  (see http://tracker.stone.com/issues/17558)
    stone osd pool create foo 8
    rados -p foo bench 1 write
    wait_for_clean || return 1

    # nuke monstore so that it will rejoin (otherwise we get
    # "not in monmap and have been in a quorum before; must have been removed"
    rm -rf $dir/aa

    # add a back in
    # (use a different addr to avoid bind issues)
    run_mon $dir aa --public-addr $STONE_MON_C || return 1
    STONE_ARGS="$BASE_STONE_ARGS --mon-host=$STONE_MON_C,$STONE_MON_B"
    wait_for_quorum 300 2 || return 1
}

main mon-seesaw "$@"

# Local Variables:
# compile-command: "cd ../.. ; make -j4 && test/mon/mon-ping.sh"
# End:
