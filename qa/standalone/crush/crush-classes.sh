#!/usr/bin/env bash
#
# Copyright (C) 2017 Red Hat <contact@redhat.com>
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

    export STONE_MON="127.0.0.1:7130" # git grep '\<7130\>' : there must be only one
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "
    STONE_ARGS+="--mon-host=$STONE_MON "
    #
    # Disable auto-class, so we can inject device class manually below
    #
    STONE_ARGS+="--osd-class-update-on-start=false "

    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        setup $dir || return 1
        $func $dir || return 1
        teardown $dir || return 1
    done
}

function add_something() {
    local dir=$1
    local obj=${2:-SOMETHING}

    local payload=ABCDEF
    echo $payload > $dir/ORIGINAL
    rados --pool rbd put $obj $dir/ORIGINAL || return 1
}

function get_osds_up() {
    local poolname=$1
    local objectname=$2

    local osds=$(stone --format xml osd map $poolname $objectname 2>/dev/null | \
        $XMLSTARLET sel -t -m "//up/osd" -v . -o ' ')
    # get rid of the trailing space
    echo $osds
}

function TEST_reweight_vs_classes() {
    local dir=$1

    # CrushWrapper::update_item (and stone osd crush set) must rebuild the shadow
    # tree too. https://tracker.stone.com/issues/48065

    run_mon $dir a || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1

    stone osd crush set-device-class ssd osd.0 || return 1
    stone osd crush class ls-osd ssd | grep 0 || return 1
    stone osd crush set-device-class ssd osd.1 || return 1
    stone osd crush class ls-osd ssd | grep 1 || return 1

    stone osd crush reweight osd.0 1

    h=`hostname -s`
    stone osd crush dump | jq ".buckets[] | select(.name==\"$h\") | .items[0].weight" | grep 65536
    stone osd crush dump | jq ".buckets[] | select(.name==\"$h~ssd\") | .items[0].weight" | grep 65536

    stone osd crush set 0 2 host=$h

    stone osd crush dump | jq ".buckets[] | select(.name==\"$h\") | .items[0].weight" | grep 131072
    stone osd crush dump | jq ".buckets[] | select(.name==\"$h~ssd\") | .items[0].weight" | grep 131072
}

function TEST_classes() {
    local dir=$1

    run_mon $dir a || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1
    create_rbd_pool || return 1

    test "$(get_osds_up rbd SOMETHING)" == "1 2 0" || return 1
    add_something $dir SOMETHING || return 1

    #
    # osd.0 has class ssd and the rule is modified
    # to only take ssd devices.
    #
    stone osd getcrushmap > $dir/map || return 1
    crushtool -d $dir/map -o $dir/map.txt || return 1
    ${SED} -i \
        -e '/device 0 osd.0/s/$/ class ssd/' \
        -e '/step take default/s/$/ class ssd/' \
        $dir/map.txt || return 1
    crushtool -c $dir/map.txt -o $dir/map-new || return 1
    stone osd setcrushmap -i $dir/map-new || return 1

    #
    # There can only be one mapping since there only is
    # one device with ssd class.
    #
    ok=false
    for delay in 2 4 8 16 32 64 128 256 ; do
        if test "$(get_osds_up rbd SOMETHING_ELSE)" == "0" ; then
            ok=true
            break
        fi
        sleep $delay
        stone osd dump # for debugging purposes
        stone pg dump # for debugging purposes
    done
    $ok || return 1
    #
    # Writing keeps working because the pool is min_size 1 by
    # default.
    #
    add_something $dir SOMETHING_ELSE || return 1

    #
    # Sanity check that the rule indeed has ssd
    # generated bucket with a name including ~ssd.
    #
    stone osd crush dump | grep -q '~ssd' || return 1
}

function TEST_set_device_class() {
    local dir=$1

    TEST_classes $dir || return 1

    stone osd crush set-device-class ssd osd.0 || return 1
    stone osd crush class ls-osd ssd | grep 0 || return 1
    stone osd crush set-device-class ssd osd.1 || return 1
    stone osd crush class ls-osd ssd | grep 1 || return 1
    stone osd crush set-device-class ssd 0 1 || return 1 # should be idempotent

    ok=false
    for delay in 2 4 8 16 32 64 128 256 ; do
        if test "$(get_osds_up rbd SOMETHING_ELSE)" == "0 1" ; then
            ok=true
            break
        fi
        sleep $delay
        stone osd crush dump
        stone osd dump # for debugging purposes
        stone pg dump # for debugging purposes
    done
    $ok || return 1
}

function TEST_mon_classes() {
    local dir=$1

    run_mon $dir a || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1
    create_rbd_pool || return 1

    test "$(get_osds_up rbd SOMETHING)" == "1 2 0" || return 1
    add_something $dir SOMETHING || return 1

    # test create and remove class
    stone osd crush class create CLASS || return 1
    stone osd crush class create CLASS || return 1 # idempotent
    stone osd crush class ls | grep CLASS  || return 1
    stone osd crush class rename CLASS TEMP || return 1
    stone osd crush class ls | grep TEMP || return 1
    stone osd crush class rename TEMP CLASS || return 1
    stone osd crush class ls | grep CLASS  || return 1
    stone osd erasure-code-profile set myprofile plugin=jerasure technique=reed_sol_van k=2 m=1 crush-failure-domain=osd crush-device-class=CLASS || return 1
    expect_failure $dir EBUSY stone osd crush class rm CLASS || return 1
    stone osd erasure-code-profile rm myprofile || return 1
    stone osd crush class rm CLASS || return 1
    stone osd crush class rm CLASS || return 1 # test idempotence

    # test rm-device-class
    stone osd crush set-device-class aaa osd.0 || return 1
    stone osd tree | grep -q 'aaa' || return 1
    stone osd crush dump | grep -q '~aaa' || return 1
    stone osd crush tree --show-shadow | grep -q '~aaa' || return 1
    stone osd crush set-device-class bbb osd.1 || return 1
    stone osd tree | grep -q 'bbb' || return 1
    stone osd crush dump | grep -q '~bbb' || return 1
    stone osd crush tree --show-shadow | grep -q '~bbb' || return 1
    stone osd crush set-device-class ccc osd.2 || return 1
    stone osd tree | grep -q 'ccc' || return 1
    stone osd crush dump | grep -q '~ccc' || return 1
    stone osd crush tree --show-shadow | grep -q '~ccc' || return 1
    stone osd crush rm-device-class 0 || return 1
    stone osd tree | grep -q 'aaa' && return 1
    stone osd crush class ls | grep -q 'aaa' && return 1 # class 'aaa' should gone
    stone osd crush rm-device-class 1 || return 1
    stone osd tree | grep -q 'bbb' && return 1
    stone osd crush class ls | grep -q 'bbb' && return 1 # class 'bbb' should gone
    stone osd crush rm-device-class 2 || return 1
    stone osd tree | grep -q 'ccc' && return 1
    stone osd crush class ls | grep -q 'ccc' && return 1 # class 'ccc' should gone
    stone osd crush set-device-class asdf all || return 1
    stone osd tree | grep -q 'asdf' || return 1
    stone osd crush dump | grep -q '~asdf' || return 1
    stone osd crush tree --show-shadow | grep -q '~asdf' || return 1
    stone osd crush rule create-replicated asdf-rule default host asdf || return 1
    stone osd crush rm-device-class all || return 1
    stone osd tree | grep -q 'asdf' && return 1
    stone osd crush class ls | grep -q 'asdf' || return 1 # still referenced by asdf-rule

    stone osd crush set-device-class abc osd.2 || return 1
    stone osd crush move osd.2 root=foo rack=foo-rack host=foo-host || return 1
    out=`stone osd tree |awk '$1 == 2 && $2 == "abc" {print $0}'`
    if [ "$out" == "" ]; then
        return 1
    fi

    # verify 'crush move' too
    stone osd crush dump | grep -q 'foo~abc' || return 1
    stone osd crush tree --show-shadow | grep -q 'foo~abc' || return 1
    stone osd crush dump | grep -q 'foo-rack~abc' || return 1
    stone osd crush tree --show-shadow | grep -q 'foo-rack~abc' || return 1
    stone osd crush dump | grep -q 'foo-host~abc' || return 1
    stone osd crush tree --show-shadow | grep -q 'foo-host~abc' || return 1
    stone osd crush rm-device-class osd.2 || return 1
    # restore class, so we can continue to test create-replicated
    stone osd crush set-device-class abc osd.2 || return 1

    stone osd crush rule create-replicated foo-rule foo host abc || return 1

    # test set-device-class implicitly change class
    stone osd crush set-device-class hdd osd.0 || return 1
    expect_failure $dir EBUSY stone osd crush set-device-class nvme osd.0 || return 1

    # test class rename
    stone osd crush rm-device-class all || return 1
    stone osd crush set-device-class class_1 all || return 1
    stone osd crush class ls | grep 'class_1' || return 1
    stone osd crush tree --show-shadow | grep 'class_1' || return 1
    stone osd crush rule create-replicated class_1_rule default host class_1 || return 1
    stone osd crush class rename class_1 class_2
    stone osd crush class rename class_1 class_2 # idempotent
    stone osd crush class ls | grep 'class_1' && return 1
    stone osd crush tree --show-shadow | grep 'class_1' && return 1
    stone osd crush class ls | grep 'class_2' || return 1
    stone osd crush tree --show-shadow | grep 'class_2' || return 1
}

main crush-classes "$@"

# Local Variables:
# compile-command: "cd ../../../build ; ln -sf ../src/stone-disk/stone_disk/main.py bin/stone-disk && make -j4 && ../src/test/crush/crush-classes.sh"
# End:
