#!/usr/bin/env bash

source $STONE_ROOT/qa/standalone/stone-helpers.sh

function run() {
    local dir=$1
    shift

    export STONE_MON_A="127.0.0.1:7150" # git grep '\<7150\>' : there must be only one
    export STONE_MON_B="127.0.0.1:7151" # git grep '\<7151\>' : there must be only one
    export STONE_MON_C="127.0.0.1:7152" # git grep '\<7152\>' : there must be only one
    export STONE_MON_D="127.0.0.1:7153" # git grep '\<7153\>' : there must be only one
    export STONE_MON_E="127.0.0.1:7154" # git grep '\<7154\>' : there must be only one
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "
    export ORIG_STONE_ARGS="$STONE_ARGS"

    local funcs=${@:-$(set | ${SED} -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
        $func $dir || return 1
        kill_daemons $dir KILL || return 1
        teardown $dir || return 1
    done
}

function TEST_1_mon_checks() {
    local dir=$1

    STONE_ARGS="$ORIG_STONE_ARGS --mon-host=$STONE_MON_A "

    run_mon $dir a --public-addr=$STONE_MON_A || return 1

    stone mon ok-to-stop dne || return 1
    ! stone mon ok-to-stop a || return 1

    ! stone mon ok-to-add-offline || return 1

    ! stone mon ok-to-rm a || return 1
    stone mon ok-to-rm dne || return 1
}

function TEST_2_mons_checks() {
    local dir=$1

    STONE_ARGS="$ORIG_STONE_ARGS --mon-host=$STONE_MON_A,$STONE_MON_B "

    run_mon $dir a --public-addr=$STONE_MON_A || return 1
    run_mon $dir b --public-addr=$STONE_MON_B || return 1

    stone mon ok-to-stop dne || return 1
    ! stone mon ok-to-stop a || return 1
    ! stone mon ok-to-stop b || return 1
    ! stone mon ok-to-stop a b || return 1

    stone mon ok-to-add-offline || return 1

    stone mon ok-to-rm a || return 1
    stone mon ok-to-rm b || return 1
    stone mon ok-to-rm dne || return 1
}

function TEST_3_mons_checks() {
    local dir=$1

    STONE_ARGS="$ORIG_STONE_ARGS --mon-host=$STONE_MON_A,$STONE_MON_B,$STONE_MON_C "

    run_mon $dir a --public-addr=$STONE_MON_A || return 1
    run_mon $dir b --public-addr=$STONE_MON_B || return 1
    run_mon $dir c --public-addr=$STONE_MON_C || return 1
    wait_for_quorum 60 3

    stone mon ok-to-stop dne || return 1
    stone mon ok-to-stop a || return 1
    stone mon ok-to-stop b || return 1
    stone mon ok-to-stop c || return 1
    ! stone mon ok-to-stop a b || return 1
    ! stone mon ok-to-stop b c || return 1
    ! stone mon ok-to-stop a b c || return 1

    stone mon ok-to-add-offline || return 1

    stone mon ok-to-rm a || return 1
    stone mon ok-to-rm b || return 1
    stone mon ok-to-rm c || return 1

    kill_daemons $dir KILL mon.b
    wait_for_quorum 60 2

    ! stone mon ok-to-stop a || return 1
    stone mon ok-to-stop b || return 1
    ! stone mon ok-to-stop c || return 1

    ! stone mon ok-to-add-offline || return 1

    ! stone mon ok-to-rm a || return 1
    stone mon ok-to-rm b || return 1
    ! stone mon ok-to-rm c || return 1
}

function TEST_4_mons_checks() {
    local dir=$1

    STONE_ARGS="$ORIG_STONE_ARGS --mon-host=$STONE_MON_A,$STONE_MON_B,$STONE_MON_C,$STONE_MON_D "

    run_mon $dir a --public-addr=$STONE_MON_A || return 1
    run_mon $dir b --public-addr=$STONE_MON_B || return 1
    run_mon $dir c --public-addr=$STONE_MON_C || return 1
    run_mon $dir d --public-addr=$STONE_MON_D || return 1
    wait_for_quorum 60 4

    stone mon ok-to-stop dne || return 1
    stone mon ok-to-stop a || return 1
    stone mon ok-to-stop b || return 1
    stone mon ok-to-stop c || return 1
    stone mon ok-to-stop d || return 1
    ! stone mon ok-to-stop a b || return 1
    ! stone mon ok-to-stop c d || return 1

    stone mon ok-to-add-offline || return 1

    stone mon ok-to-rm a || return 1
    stone mon ok-to-rm b || return 1
    stone mon ok-to-rm c || return 1

    kill_daemons $dir KILL mon.a
    wait_for_quorum 60 3

    stone mon ok-to-stop a || return 1
    ! stone mon ok-to-stop b || return 1
    ! stone mon ok-to-stop c || return 1
    ! stone mon ok-to-stop d || return 1

    stone mon ok-to-add-offline || return 1

    stone mon ok-to-rm a || return 1
    stone mon ok-to-rm b || return 1
    stone mon ok-to-rm c || return 1
    stone mon ok-to-rm d || return 1
}

function TEST_5_mons_checks() {
    local dir=$1

    STONE_ARGS="$ORIG_STONE_ARGS --mon-host=$STONE_MON_A,$STONE_MON_B,$STONE_MON_C,$STONE_MON_D,$STONE_MON_E "

    run_mon $dir a --public-addr=$STONE_MON_A || return 1
    run_mon $dir b --public-addr=$STONE_MON_B || return 1
    run_mon $dir c --public-addr=$STONE_MON_C || return 1
    run_mon $dir d --public-addr=$STONE_MON_D || return 1
    run_mon $dir e --public-addr=$STONE_MON_E || return 1
    wait_for_quorum 60 5

    stone mon ok-to-stop dne || return 1
    stone mon ok-to-stop a || return 1
    stone mon ok-to-stop b || return 1
    stone mon ok-to-stop c || return 1
    stone mon ok-to-stop d || return 1
    stone mon ok-to-stop e || return 1
    stone mon ok-to-stop a b || return 1
    stone mon ok-to-stop c d || return 1
    ! stone mon ok-to-stop a b c || return 1

    stone mon ok-to-add-offline || return 1

    stone mon ok-to-rm a || return 1
    stone mon ok-to-rm b || return 1
    stone mon ok-to-rm c || return 1
    stone mon ok-to-rm d || return 1
    stone mon ok-to-rm e || return 1

    kill_daemons $dir KILL mon.a
    wait_for_quorum 60 4

    stone mon ok-to-stop a || return 1
    stone mon ok-to-stop b || return 1
    stone mon ok-to-stop c || return 1
    stone mon ok-to-stop d || return 1
    stone mon ok-to-stop e || return 1

    stone mon ok-to-add-offline || return 1

    stone mon ok-to-rm a || return 1
    stone mon ok-to-rm b || return 1
    stone mon ok-to-rm c || return 1
    stone mon ok-to-rm d || return 1
    stone mon ok-to-rm e || return 1

    kill_daemons $dir KILL mon.e
    wait_for_quorum 60 3

    stone mon ok-to-stop a || return 1
    ! stone mon ok-to-stop b || return 1
    ! stone mon ok-to-stop c || return 1
    ! stone mon ok-to-stop d || return 1
    stone mon ok-to-stop e || return 1

    ! stone mon ok-to-add-offline || return 1

    stone mon ok-to-rm a || return 1
    ! stone mon ok-to-rm b || return 1
    ! stone mon ok-to-rm c || return 1
    ! stone mon ok-to-rm d || return 1
    stone mon ok-to-rm e || return 1
}

function TEST_0_mds() {
    local dir=$1

    STONE_ARGS="$ORIG_STONE_ARGS --mon-host=$STONE_MON_A "

    run_mon $dir a --public-addr=$STONE_MON_A || return 1
    run_mgr $dir x || return 1
    run_osd $dir 0 || return 1
    run_mds $dir a || return 1

    stone osd pool create meta 1 || return 1
    stone osd pool create data 1 || return 1
    stone fs new myfs meta data || return 1
    sleep 5

    ! stone mds ok-to-stop a || return 1
    ! stone mds ok-to-stop a dne || return 1
    stone mds ok-to-stop dne || return 1

    run_mds $dir b || return 1
    sleep 5

    stone mds ok-to-stop a || return 1
    stone mds ok-to-stop b || return 1
    ! stone mds ok-to-stop a b || return 1
    stone mds ok-to-stop a dne1 dne2 || return 1
    stone mds ok-to-stop b dne || return 1
    ! stone mds ok-to-stop a b dne || return 1
    stone mds ok-to-stop dne1 dne2 || return 1

    kill_daemons $dir KILL mds.a
}

function TEST_0_osd() {
    local dir=$1

    STONE_ARGS="$ORIG_STONE_ARGS --mon-host=$STONE_MON_A "

    run_mon $dir a --public-addr=$STONE_MON_A || return 1
    run_mgr $dir x || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1
    run_osd $dir 3 || return 1

    stone osd erasure-code-profile set ec-profile m=2 k=2 crush-failure-domain=osd || return 1
    stone osd pool create ec erasure ec-profile || return 1

    wait_for_clean || return 1

    # with min_size 3, we can stop only 1 osd
    stone osd pool set ec min_size 3 || return 1
    wait_for_clean || return 1

    stone osd ok-to-stop 0 || return 1
    stone osd ok-to-stop 1 || return 1
    stone osd ok-to-stop 2 || return 1
    stone osd ok-to-stop 3 || return 1
    ! stone osd ok-to-stop 0 1 || return 1
    ! stone osd ok-to-stop 2 3 || return 1
    stone osd ok-to-stop 0 --max 2 | grep '[0]' || return 1
    stone osd ok-to-stop 1 --max 2 | grep '[1]' || return 1

    # with min_size 2 we can stop 1 osds
    stone osd pool set ec min_size 2 || return 1
    wait_for_clean || return 1

    stone osd ok-to-stop 0 1 || return 1
    stone osd ok-to-stop 2 3 || return 1
    ! stone osd ok-to-stop 0 1 2 || return 1
    ! stone osd ok-to-stop 1 2 3 || return 1

    stone osd ok-to-stop 0 --max 2 | grep '[0,1]' || return 1
    stone osd ok-to-stop 0 --max 20 | grep '[0,1]' || return 1
    stone osd ok-to-stop 2 --max 2 | grep '[2,3]' || return 1
    stone osd ok-to-stop 2 --max 20 | grep '[2,3]' || return 1

    # we should get the same result with one of the osds already down
    kill_daemons $dir TERM osd.0 || return 1
    stone osd down 0 || return 1
    wait_for_peered || return 1

    stone osd ok-to-stop 0 || return 1
    stone osd ok-to-stop 0 1 || return 1
    ! stone osd ok-to-stop 0 1 2 || return 1
    ! stone osd ok-to-stop 1 2 3 || return 1
}


main ok-to-stop "$@"
