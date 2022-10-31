#!/usr/bin/env bash
#
# Copyright (C) 2013 Cloudwatt <libre.licensing@cloudwatt.com>
# Copyright (C) 2014 Red Hat <contact@redhat.com>
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
set -xe
PS4='${BASH_SOURCE[0]}:$LINENO: ${FUNCNAME[0]}:  '


DIR=mkfs
export STONE_CONF=/dev/null
unset STONE_ARGS
MON_ID=a
MON_DIR=$DIR/$MON_ID
STONE_MON=127.0.0.1:7110 # git grep '\<7110\>' : there must be only one
TIMEOUT=360

EXTRAOPTS=""

function setup() {
    teardown
    mkdir $DIR
}

function teardown() {
    kill_daemons
    rm -fr $DIR
}

function mon_mkfs() {
    local fsid=$(uuidgen)

    stone-mon \
        --id $MON_ID \
        --fsid $fsid \
	$EXTRAOPTS \
        --mkfs \
        --mon-data=$MON_DIR \
        --mon-initial-members=$MON_ID \
        --mon-host=$STONE_MON \
        "$@"
}

function mon_run() {
    stone-mon \
        --id $MON_ID \
        --chdir= \
        --mon-osd-full-ratio=.99 \
        --mon-data-avail-crit=1 \
	$EXTRAOPTS \
        --mon-data=$MON_DIR \
        --log-file=$MON_DIR/log \
        --mon-cluster-log-file=$MON_DIR/log \
        --run-dir=$MON_DIR \
        --pid-file=$MON_DIR/pidfile \
        --public-addr $STONE_MON \
        "$@"
}

function kill_daemons() {
    for pidfile in $(find $DIR -name pidfile) ; do
        pid=$(cat $pidfile)
        for try in 0 1 1 1 2 3 ; do
            kill $pid || break
            sleep $try
        done
    done
}

function auth_none() {
    mon_mkfs --auth-supported=none

    stone-mon \
        --id $MON_ID \
        --mon-osd-full-ratio=.99 \
        --mon-data-avail-crit=1 \
	$EXTRAOPTS \
        --mon-data=$MON_DIR \
        --extract-monmap $MON_DIR/monmap

    [ -f $MON_DIR/monmap ] || return 1

    [ ! -f $MON_DIR/keyring ] || return 1

    mon_run --auth-supported=none

    timeout $TIMEOUT stone --mon-host $STONE_MON mon stat || return 1
}

function auth_stonex_keyring() {
    cat > $DIR/keyring <<EOF
[mon.]
	key = AQDUS79S0AF9FRAA2cgRLFscVce0gROn/s9WMg==
	caps mon = "allow *"
EOF

    mon_mkfs --keyring=$DIR/keyring

    [ -f $MON_DIR/keyring ] || return 1

    mon_run

    timeout $TIMEOUT stone \
        --name mon. \
        --keyring $MON_DIR/keyring \
        --mon-host $STONE_MON mon stat || return 1
}

function auth_stonex_key() {
    if [ -f /etc/stonepros/keyring ] ; then
	echo "Please move /etc/stonepros/keyring away for testing!"
	return 1
    fi

    local key=$(stone-authtool --gen-print-key)

    if mon_mkfs --key='corrupted key' ; then
        return 1
    else
        rm -fr $MON_DIR/store.db
        rm -fr $MON_DIR/kv_backend
    fi

    mon_mkfs --key=$key

    [ -f $MON_DIR/keyring ] || return 1
    grep $key $MON_DIR/keyring

    mon_run

    timeout $TIMEOUT stone \
        --name mon. \
        --keyring $MON_DIR/keyring \
        --mon-host $STONE_MON mon stat || return 1
}

function makedir() {
    local toodeep=$MON_DIR/toodeep

    # fail if recursive directory creation is needed
    stone-mon \
        --id $MON_ID \
        --mon-osd-full-ratio=.99 \
        --mon-data-avail-crit=1 \
	$EXTRAOPTS \
        --mkfs \
        --mon-data=$toodeep 2>&1 | tee $DIR/makedir.log
    grep 'toodeep.*No such file' $DIR/makedir.log > /dev/null
    rm $DIR/makedir.log

    # an empty directory does not mean the mon exists
    mkdir $MON_DIR
    mon_mkfs --auth-supported=none 2>&1 | tee $DIR/makedir.log
    ! grep "$MON_DIR already exists" $DIR/makedir.log || return 1
}

function idempotent() {
    mon_mkfs --auth-supported=none
    mon_mkfs --auth-supported=none 2>&1 | tee $DIR/makedir.log
    grep "'$MON_DIR' already exists" $DIR/makedir.log > /dev/null || return 1
}

function run() {
    local actions
    actions+="makedir "
    actions+="idempotent "
    actions+="auth_stonex_key "
    actions+="auth_stonex_keyring "
    actions+="auth_none "
    for action in $actions  ; do
        setup
        $action || return 1
        teardown
    done
}

run

# Local Variables:
# compile-command: "cd ../.. ; make TESTS=test/mon/mkfs.sh check"
# End:
