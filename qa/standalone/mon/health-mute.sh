#!/bin/bash

source $STONE_ROOT/qa/standalone/stone-helpers.sh

function run() {
    local dir=$1
    shift

    export STONE_MON="127.0.0.1:7143" # git grep '\<714\>' : there must be only one
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none --mon-pg-warn-min-per-osd 0 --mon-max-pg-per-osd 1000 "
    STONE_ARGS+="--mon-host=$STONE_MON "

    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
        $func $dir || return 1
        teardown $dir || return 1
    done
}

function TEST_mute() {
    local dir=$1
    setup $dir || return 1

    set -o pipefail

    run_mon $dir a || return 1
    run_mgr $dir x || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1

    stone osd pool create foo 8
    stone osd pool application enable foo rbd --yes-i-really-mean-it
    wait_for_clean || return 1

    stone -s
    stone health | grep HEALTH_OK || return 1
    # test warning on setting pool size=1
    stone osd pool set foo size 1 --yes-i-really-mean-it
    stone -s
    stone health | grep HEALTH_WARN || return 1
    stone health detail | grep POOL_NO_REDUNDANCY || return 1
    stone health mute POOL_NO_REDUNDANCY
    stone -s
    stone health | grep HEALTH_OK | grep POOL_NO_REDUNDANCY || return 1
    stone health unmute POOL_NO_REDUNDANCY
    stone -s
    stone health | grep HEALTH_WARN || return 1
    # restore pool size to default
    stone osd pool set foo size 3
    stone -s
    stone health | grep HEALTH_OK || return 1
    stone osd set noup
    stone -s
    stone health detail | grep OSDMAP_FLAGS || return 1
    stone osd down 0
    stone -s
    stone health detail | grep OSD_DOWN || return 1
    stone health detail | grep HEALTH_WARN || return 1

    stone health mute OSD_DOWN
    stone health mute OSDMAP_FLAGS
    stone -s
    stone health | grep HEALTH_OK | grep OSD_DOWN | grep OSDMAP_FLAGS || return 1
    stone health unmute OSD_DOWN
    stone -s
    stone health | grep HEALTH_WARN || return 1

    # ttl
    stone health mute OSD_DOWN 10s
    stone -s
    stone health | grep HEALTH_OK || return 1
    sleep 15
    stone -s
    stone health | grep HEALTH_WARN || return 1

    # sticky
    stone health mute OSDMAP_FLAGS --sticky
    stone osd unset noup
    sleep 5
    stone -s
    stone health | grep OSDMAP_FLAGS || return 1
    stone osd set noup
    stone -s
    stone health | grep HEALTH_OK || return 1

    # rachet down on OSD_DOWN count
    stone osd down 0 1
    stone -s
    stone health detail | grep OSD_DOWN || return 1

    stone health mute OSD_DOWN
    kill_daemons $dir TERM osd.0
    stone osd unset noup
    sleep 10
    stone -s
    stone health detail | grep OSD_DOWN || return 1
    stone health detail | grep '1 osds down' || return 1
    stone health | grep HEALTH_OK || return 1

    sleep 10 # give time for mon tick to rachet the mute
    stone osd set noup
    stone health mute OSDMAP_FLAGS
    stone -s
    stone health detail
    stone health | grep HEALTH_OK || return 1

    stone osd down 1
    stone -s
    stone health detail
    stone health detail | grep '2 osds down' || return 1

    sleep 10 # give time for mute to clear
    stone -s
    stone health detail
    stone health | grep HEALTH_WARN || return 1
    stone health detail | grep '2 osds down' || return 1

    teardown $dir || return 1
}

main health-mute "$@"
