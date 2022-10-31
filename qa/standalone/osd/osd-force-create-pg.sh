#!/usr/bin/env bash
source $STONE_ROOT/qa/standalone/stone-helpers.sh

function run() {
    local dir=$1
    shift

    export STONE_MON="127.0.0.1:7145" # git grep '\<7145\>' : there must be only one
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "
    STONE_ARGS+="--mon-host=$STONE_MON "

    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
        $func $dir || return 1
        teardown $dir || return 1
    done
}

function TEST_reuse_id() {
    local dir=$1

    run_mon $dir a --osd_pool_default_size=1 --mon_allow_pool_size_one=true || return 1
    run_mgr $dir x || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1

    stone osd pool create foo 50 || return 1
    wait_for_clean || return 1

    kill_daemons $dir TERM osd.0
    kill_daemons $dir TERM osd.1
    kill_daemons $dir TERM osd.2
    stone-objectstore-tool --data-path $dir/0 --op remove --pgid 1.0  --force
    stone-objectstore-tool --data-path $dir/1 --op remove --pgid 1.0  --force
    stone-objectstore-tool --data-path $dir/2 --op remove --pgid 1.0  --force
    activate_osd $dir 0 || return 1
    activate_osd $dir 1 || return 1
    activate_osd $dir 2 || return 1
    sleep 10
    stone pg ls | grep 1.0 | grep stale || return 1

    stone osd force-create-pg 1.0 --yes-i-really-mean-it || return 1
    wait_for_clean || return 1
}

main osd-force-create-pg "$@"

# Local Variables:
# compile-command: "cd ../.. ; make -j4 && test/osd/osd-force-create-pg.sh"
# End:
