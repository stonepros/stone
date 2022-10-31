#!/usr/bin/env bash
#
# Copyright (C) 2013, 2014 Cloudwatt <libre.licensing@cloudwatt.com>
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

    export STONE_MON="127.0.0.1:7105" # git grep '\<7105\>' : there must be only one
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "
    STONE_ARGS+="--mon-host=$STONE_MON "
    export STONE_ARGS

    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
        $func $dir || return 1
        teardown $dir || return 1
    done
}

# Before http://tracker.stone.com/issues/8307 the invalid profile was created
function TEST_erasure_invalid_profile() {
    local dir=$1
    run_mon $dir a || return 1
    local poolname=pool_erasure
    local notaprofile=not-a-valid-erasure-code-profile
    ! stone osd pool create $poolname 12 12 erasure $notaprofile || return 1
    ! stone osd erasure-code-profile ls | grep $notaprofile || return 1
}

function TEST_erasure_crush_rule() {
    local dir=$1
    run_mon $dir a || return 1
    #
    # choose the crush ruleset used with an erasure coded pool
    #
    local crush_ruleset=myruleset
    ! stone osd crush rule ls | grep $crush_ruleset || return 1
    stone osd crush rule create-erasure $crush_ruleset
    stone osd crush rule ls | grep $crush_ruleset
    local poolname
    poolname=pool_erasure1
    ! stone --format json osd dump | grep '"crush_rule":1' || return 1
    stone osd pool create $poolname 12 12 erasure default $crush_ruleset
    stone --format json osd dump | grep '"crush_rule":1' || return 1
    #
    # a crush ruleset by the name of the pool is implicitly created
    #
    poolname=pool_erasure2
    stone osd erasure-code-profile set myprofile
    stone osd pool create $poolname 12 12 erasure myprofile
    stone osd crush rule ls | grep $poolname || return 1
    #
    # a non existent crush ruleset given in argument is an error
    # http://tracker.stone.com/issues/9304
    #
    poolname=pool_erasure3
    ! stone osd pool create $poolname 12 12 erasure myprofile INVALIDRULESET || return 1
}

function TEST_erasure_code_profile_default() {
    local dir=$1
    run_mon $dir a || return 1
    stone osd erasure-code-profile rm default || return 1
    ! stone osd erasure-code-profile ls | grep default || return 1
    stone osd pool create $poolname 12 12 erasure default
    stone osd erasure-code-profile ls | grep default || return 1
}

function TEST_erasure_crush_stripe_unit() {
    local dir=$1
    # the default stripe unit is used to initialize the pool
    run_mon $dir a --public-addr $STONE_MON
    stripe_unit=$(stone-conf --show-config-value osd_pool_erasure_code_stripe_unit)
    eval local $(stone osd erasure-code-profile get myprofile | grep k=)
    stripe_width = $((stripe_unit * k))
    stone osd pool create pool_erasure 12 12 erasure
    stone --format json osd dump | tee $dir/osd.json
    grep '"stripe_width":'$stripe_width $dir/osd.json > /dev/null || return 1
}

function TEST_erasure_crush_stripe_unit_padded() {
    local dir=$1
    # setting osd_pool_erasure_code_stripe_unit modifies the stripe_width
    # and it is padded as required by the default plugin
    profile+=" plugin=jerasure"
    profile+=" technique=reed_sol_van"
    k=4
    profile+=" k=$k"
    profile+=" m=2"
    actual_stripe_unit=2048
    desired_stripe_unit=$((actual_stripe_unit - 1))
    actual_stripe_width=$((actual_stripe_unit * k))
    run_mon $dir a \
        --osd_pool_erasure_code_stripe_unit $desired_stripe_unit \
        --osd_pool_default_erasure_code_profile "$profile" || return 1
    stone osd pool create pool_erasure 12 12 erasure
    stone osd dump | tee $dir/osd.json
    grep "stripe_width $actual_stripe_width" $dir/osd.json > /dev/null || return 1
}

function TEST_erasure_code_pool() {
    local dir=$1
    run_mon $dir a || return 1
    stone --format json osd dump > $dir/osd.json
    local expected='"erasure_code_profile":"default"'
    ! grep "$expected" $dir/osd.json || return 1
    stone osd pool create erasurecodes 12 12 erasure
    stone --format json osd dump | tee $dir/osd.json
    grep "$expected" $dir/osd.json > /dev/null || return 1

    stone osd pool create erasurecodes 12 12 erasure 2>&1 | \
        grep 'already exists' || return 1
    stone osd pool create erasurecodes 12 12 2>&1 | \
        grep 'cannot change to type replicated' || return 1
}

function TEST_replicated_pool_with_ruleset() {
    local dir=$1
    run_mon $dir a
    local ruleset=ruleset0
    local root=host1
    stone osd crush add-bucket $root host
    local failure_domain=osd
    local poolname=mypool
    stone osd crush rule create-simple $ruleset $root $failure_domain || return 1
    stone osd crush rule ls | grep $ruleset
    stone osd pool create $poolname 12 12 replicated $ruleset || return 1
    rule_id=`stone osd crush rule dump $ruleset | grep "rule_id" | awk -F[' ':,] '{print $4}'`
    stone osd pool get $poolname crush_rule  2>&1 | \
        grep "crush_rule: $rule_id" || return 1
    #non-existent crush ruleset
    stone osd pool create newpool 12 12 replicated non-existent 2>&1 | \
        grep "doesn't exist" || return 1
}

function TEST_erasure_code_pool_lrc() {
    local dir=$1
    run_mon $dir a || return 1

    stone osd erasure-code-profile set LRCprofile \
             plugin=lrc \
             mapping=DD_ \
             layers='[ [ "DDc", "" ] ]' || return 1

    stone --format json osd dump > $dir/osd.json
    local expected='"erasure_code_profile":"LRCprofile"'
    local poolname=erasurecodes
    ! grep "$expected" $dir/osd.json || return 1
    stone osd pool create $poolname 12 12 erasure LRCprofile
    stone --format json osd dump | tee $dir/osd.json
    grep "$expected" $dir/osd.json > /dev/null || return 1
    stone osd crush rule ls | grep $poolname || return 1
}

function TEST_replicated_pool() {
    local dir=$1
    run_mon $dir a || return 1
    stone osd pool create replicated 12 12 replicated replicated_rule || return 1
    stone osd pool create replicated 12 12 replicated replicated_rule 2>&1 | \
        grep 'already exists' || return 1
    # default is replicated
    stone osd pool create replicated1 12 12 || return 1
    # default is replicated, pgp_num = pg_num
    stone osd pool create replicated2 12 || return 1
    stone osd pool create replicated 12 12 erasure 2>&1 | \
        grep 'cannot change to type erasure' || return 1
}

function TEST_no_pool_delete() {
    local dir=$1
    run_mon $dir a || return 1
    stone osd pool create foo 1 || return 1
    stone tell mon.a injectargs -- --no-mon-allow-pool-delete || return 1
    ! stone osd pool delete foo foo --yes-i-really-really-mean-it || return 1
    stone tell mon.a injectargs -- --mon-allow-pool-delete || return 1
    stone osd pool delete foo foo --yes-i-really-really-mean-it || return 1
}

function TEST_utf8_cli() {
    local dir=$1
    run_mon $dir a || return 1
    # Hopefully it's safe to include literal UTF-8 characters to test
    # the fix for http://tracker.stone.com/issues/7387.  If it turns out
    # to not be OK (when is the default encoding *not* UTF-8?), maybe
    # the character '黄' can be replaced with the escape $'\xe9\xbb\x84'
    OLDLANG="$LANG"
    export LANG=en_US.UTF-8
    stone osd pool create 黄 16 || return 1
    stone osd lspools 2>&1 | \
        grep "黄" || return 1
    stone -f json-pretty osd dump | \
        python3 -c "import json; import sys; json.load(sys.stdin)" || return 1
    stone osd pool delete 黄 黄 --yes-i-really-really-mean-it
    export LANG="$OLDLANG"
}

function TEST_pool_create_rep_expected_num_objects() {
    local dir=$1
    setup $dir || return 1

    export STONE_ARGS
    run_mon $dir a || return 1
    run_mgr $dir x || return 1
    # disable pg dir merge
    run_osd_filestore $dir 0 || return 1

    stone osd pool create rep_expected_num_objects 64 64 replicated  replicated_rule 100000 || return 1
    # wait for pg dir creating
    sleep 30
    stone pg ls
    find ${dir}/0/current -ls
    ret=$(find ${dir}/0/current/1.0_head/ | grep DIR | wc -l)
    if [ "$ret" -le 2 ];
    then
        return 1
    else
        echo "TEST_pool_create_rep_expected_num_objects PASS"
    fi
}

function check_pool_priority() {
    local dir=$1
    shift
    local pools=$1
    shift
    local spread="$1"
    shift
    local results="$1"

    setup $dir || return 1

    EXTRA_OPTS="--debug_allow_any_pool_priority=true"
    export EXTRA_OPTS
    run_mon $dir a || return 1
    run_mgr $dir x || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1

    # Add pool 0 too
    for i in $(seq 0 $pools)
    do
      num=$(expr $i + 1)
      stone osd pool create test${num} 1 1
    done

    wait_for_clean || return 1
    for i in $(seq 0 $pools)
    do
	num=$(expr $i + 1)
	stone osd pool set test${num} recovery_priority $(expr $i \* $spread)
    done

    #grep "recovery_priority.*pool set" out/mon.a.log

    bin/stone osd dump

    # Restart everything so mon converts the priorities
    kill_daemons
    run_mon $dir a || return 1
    run_mgr $dir x || return 1
    activate_osd $dir 0 || return 1
    activate_osd $dir 1 || return 1
    activate_osd $dir 2 || return 1
    sleep 5

    grep convert $dir/mon.a.log
    stone osd dump

    pos=1
    for i in $(stone osd dump | grep ^pool | sed 's/.*recovery_priority //' | awk '{ print $1 }')
    do
      result=$(echo $results | awk "{ print \$${pos} }")
      # A value of 0 is an unset value so sed/awk gets "pool"
      if test $result = "0"
      then
        result="pool"
      fi
      test "$result" = "$i" || return 1
      pos=$(expr $pos + 1)
    done
}

function TEST_pool_pos_only_prio() {
   local dir=$1
   check_pool_priority $dir 20 5 "0 0 1 1 2 2 3 3 4 4 5 5 6 6 7 7 8 8 9 9 10" || return 1
}

function TEST_pool_neg_only_prio() {
   local dir=$1
   check_pool_priority $dir 20 -5 "0 0 -1 -1 -2 -2 -3 -3 -4 -4 -5 -5 -6 -6 -7 -7 -8 -8 -9 -9 -10" || return 1
}

function TEST_pool_both_prio() {
   local dir=$1
   check_pool_priority $dir 20 "5 - 50" "-10 -9 -8 -7 -6 -5 -4 -3 -2 -1 0 1 2 3 4 5 6 7 8 9 10" || return 1
}

function TEST_pool_both_prio_no_neg() {
   local dir=$1
   check_pool_priority $dir 20 "2 - 4" "-4 -2 0 0 1 1 2 2 3 3 4 5 5 6 6 7 7 8 8 9 10" || return 1
}

function TEST_pool_both_prio_no_pos() {
   local dir=$1
   check_pool_priority $dir 20 "2 - 36" "-10 -9 -8 -8 -7 -7 -6 -6 -5 -5 -4 -3 -3 -2 -2 -1 -1 0 0 2 4" || return 1
}


main osd-pool-create "$@"

# Local Variables:
# compile-command: "cd ../.. ; make -j4 && test/mon/osd-pool-create.sh"
# End:
