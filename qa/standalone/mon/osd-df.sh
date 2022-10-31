#!/bin/bash

source $STONE_ROOT/qa/standalone/stone-helpers.sh

function run() {
    local dir=$1
    shift

    export STONE_MON="127.0.0.1:7113" # git grep '\<7113\>' : there must be only one
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

function TEST_osd_df() {
    local dir=$1
    setup $dir || return 1

    run_mon $dir a || return 1
    run_mgr $dir x || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1
    run_osd $dir 3 || return 1
    run_osd $dir 4 || return 1
    run_osd $dir 5 || return 1

    # normal case
    stone osd df --f json-pretty | grep osd.0 || return 1
    stone osd df --f json-pretty | grep osd.1 || return 1
    stone osd df --f json-pretty | grep osd.2 || return 1
    stone osd df --f json-pretty | grep osd.3 || return 1
    stone osd df --f json-pretty | grep osd.4 || return 1
    stone osd df --f json-pretty | grep osd.5 || return 1

    # filter by device class
    osd_class=$(stone osd crush get-device-class 0)
    stone osd df class $osd_class --f json-pretty | grep 'osd.0' || return 1
    # post-nautilus we require filter-type no more
    stone osd df $osd_class --f json-pretty | grep 'osd.0' || return 1
    stone osd crush rm-device-class 0 || return 1
    stone osd crush set-device-class aaa 0 || return 1
    stone osd df aaa --f json-pretty | grep 'osd.0' || return 1
    stone osd df aaa --f json-pretty | grep 'osd.1' && return 1
    # reset osd.1's device class
    stone osd crush rm-device-class 0 || return 1
    stone osd crush set-device-class $osd_class 0 || return 1

    # filter by crush node
    stone osd df osd.0 --f json-pretty | grep osd.0 || return 1
    stone osd df osd.0 --f json-pretty | grep osd.1 && return 1
    stone osd crush move osd.0 root=default host=foo || return 1
    stone osd crush move osd.1 root=default host=foo || return 1
    stone osd crush move osd.2 root=default host=foo || return 1
    stone osd crush move osd.3 root=default host=bar || return 1
    stone osd crush move osd.4 root=default host=bar || return 1
    stone osd crush move osd.5 root=default host=bar || return 1
    stone osd df tree foo --f json-pretty | grep foo || return 1
    stone osd df tree foo --f json-pretty | grep bar && return 1
    stone osd df foo --f json-pretty | grep osd.0 || return 1
    stone osd df foo --f json-pretty | grep osd.1 || return 1
    stone osd df foo --f json-pretty | grep osd.2 || return 1
    stone osd df foo --f json-pretty | grep osd.3 && return 1
    stone osd df foo --f json-pretty | grep osd.4 && return 1
    stone osd df foo --f json-pretty | grep osd.5 && return 1
    stone osd df tree bar --f json-pretty | grep bar || return 1
    stone osd df tree bar --f json-pretty | grep foo && return 1
    stone osd df bar --f json-pretty | grep osd.0 && return 1
    stone osd df bar --f json-pretty | grep osd.1 && return 1
    stone osd df bar --f json-pretty | grep osd.2 && return 1
    stone osd df bar --f json-pretty | grep osd.3 || return 1
    stone osd df bar --f json-pretty | grep osd.4 || return 1
    stone osd df bar --f json-pretty | grep osd.5 || return 1

    # filter by pool
    stone osd crush rm-device-class all || return 1
    stone osd crush set-device-class nvme 0 1 3 4 || return 1
    stone osd crush rule create-replicated nvme-rule default host nvme || return 1
    stone osd pool create nvme-pool 12 12 nvme-rule || return 1
    stone osd df nvme-pool --f json-pretty | grep osd.0 || return 1
    stone osd df nvme-pool --f json-pretty | grep osd.1 || return 1
    stone osd df nvme-pool --f json-pretty | grep osd.2 && return 1
    stone osd df nvme-pool --f json-pretty | grep osd.3 || return 1
    stone osd df nvme-pool --f json-pretty | grep osd.4 || return 1
    stone osd df nvme-pool --f json-pretty | grep osd.5 && return 1

    teardown $dir || return 1
}

main osd-df "$@"
