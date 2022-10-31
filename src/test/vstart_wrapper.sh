#!/usr/bin/env bash
#
# Copyright (C) 2013 Cloudwatt <libre.licensing@cloudwatt.com>
# Copyright (C) 2015 Red Hat <contact@redhat.com>
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

export STONE_VSTART_WRAPPER=1
export STONE_DIR="${TMPDIR:-$PWD}/td/t-$STONE_PORT"
export STONE_DEV_DIR="$STONE_DIR/dev"
export STONE_OUT_DIR="$STONE_DIR/out"
export STONE_ASOK_DIR="$STONE_DIR/out"

export MGR_PYTHON_PATH=$STONE_ROOT/src/pybind/mgr

function vstart_setup()
{
    rm -fr $STONE_DEV_DIR $STONE_OUT_DIR
    mkdir -p $STONE_DEV_DIR
    trap "teardown $STONE_DIR" EXIT
    export LC_ALL=C # some tests are vulnerable to i18n
    export PATH="$(pwd):${PATH}"
    OBJSTORE_ARGS=""
    if [ "bluestore" = "${STONE_OBJECTSTORE}" ]; then
        OBJSTORE_ARGS="-b"
    fi
    $STONE_ROOT/src/vstart.sh \
        --short \
        $OBJSTORE_ARGS \
        -o 'paxos propose interval = 0.01' \
        -d -n -l || return 1
    export STONE_CONF=$STONE_DIR/stone.conf

    crit=$(expr 100 - $(stone-conf --show-config-value mon_data_avail_crit))
    if [ $(df . | perl -ne 'print if(s/.*\s(\d+)%.*/\1/)') -ge $crit ] ; then
        df . 
        cat <<EOF
error: not enough free disk space for mon to run
The mon will shutdown with a message such as 
 "reached critical levels of available space on local monitor storage -- shutdown!"
as soon as it finds the disk has is more than ${crit}% full. 
This is a limit determined by
 stone-conf --show-config-value mon_data_avail_crit
EOF
        return 1
    fi
}

function main()
{
    teardown $STONE_DIR
    vstart_setup || return 1
    if STONE_CONF=$STONE_DIR/stone.conf "$@"; then
        code=0
    else
        code=1
        display_logs $STONE_OUT_DIR
    fi
    return $code
}

main "$@"
