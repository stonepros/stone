#! /usr/bin/env bash
#
# Copyright (C) 2017 Red Hat <contact@redhat.com>
#
# Author: David Zafman <dzafman@redhat.com>
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

    export STONE_MON="127.0.0.1:7124" # git grep '\<7124\>' : there must be only one
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "
    STONE_ARGS+="--mon-host=$STONE_MON "

    export -n STONE_CLI_TEST_DUP_COMMAND
    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        $func $dir || return 1
    done
}

# Simple test for "not scheduling scrubs due to active recovery"
# OSD::sched_scrub() called on all OSDs during ticks
function TEST_recovery_scrub_1() {
    local dir=$1
    local poolname=test

    TESTDATA="testdata.$$"
    OSDS=4
    PGS=1
    OBJECTS=100
    ERRORS=0

    setup $dir || return 1
    run_mon $dir a --osd_pool_default_size=1 --mon_allow_pool_size_one=true \
                   --osd_scrub_interval_randomize_ratio=0.0 || return 1
    run_mgr $dir x || return 1
    for osd in $(seq 0 $(expr $OSDS - 1))
    do
        run_osd $dir $osd --osd_scrub_during_recovery=false || return 1
    done

    # Create a pool with $PGS pgs
    create_pool $poolname $PGS $PGS
    wait_for_clean || return 1
    poolid=$(stone osd dump | grep "^pool.*[']test[']" | awk '{ print $2 }')

    stone pg dump pgs

    dd if=/dev/urandom of=$TESTDATA bs=1M count=50
    for i in $(seq 1 $OBJECTS)
    do
        rados -p $poolname put obj${i} $TESTDATA
    done
    rm -f $TESTDATA

    stone osd pool set $poolname size 4

    # Wait for recovery to start
    set -o pipefail
    count=0
    while(true)
    do
      if stone --format json pg dump pgs |
        jq '.pg_stats | [.[] | .state | contains("recovering")]' | grep -q true
      then
        break
      fi
      sleep 2
      if test "$count" -eq "10"
      then
        echo "Recovery never started"
        return 1
      fi
      count=$(expr $count + 1)
    done
    set +o pipefail
    stone pg dump pgs

    sleep 10
    # Work around for http://tracker.stone.com/issues/38195
    kill_daemons $dir #|| return 1

    declare -a err_strings
    err_strings[0]="not scheduling scrubs due to active recovery"

    for osd in $(seq 0 $(expr $OSDS - 1))
    do
        grep "not scheduling scrubs" $dir/osd.${osd}.log
    done
    for err_string in "${err_strings[@]}"
    do
        found=false
	count=0
        for osd in $(seq 0 $(expr $OSDS - 1))
        do
            if grep -q "$err_string" $dir/osd.${osd}.log
            then
                found=true
		count=$(expr $count + 1)
            fi
        done
        if [ "$found" = "false" ]; then
            echo "Missing log message '$err_string'"
            ERRORS=$(expr $ERRORS + 1)
        fi
        [ $count -eq $OSDS ] || return 1
    done

    teardown $dir || return 1

    if [ $ERRORS != "0" ];
    then
        echo "TEST FAILED WITH $ERRORS ERRORS"
        return 1
    fi

    echo "TEST PASSED"
    return 0
}

##
# a modified version of wait_for_scrub(), which terminates if the Primary
# of the to-be-scrubbed PG changes
#
# Given the *last_scrub*, wait for scrub to happen on **pgid**. It
# will fail if scrub does not complete within $TIMEOUT seconds. The
# repair is complete whenever the **get_last_scrub_stamp** function
# reports a timestamp different from the one given in argument.
#
# @param pgid the id of the PG
# @param the primary OSD when started
# @param last_scrub timestamp of the last scrub for *pgid*
# @return 0 on success, 1 on error
#
function wait_for_scrub_mod() {
    local pgid=$1
    local orig_primary=$2
    local last_scrub="$3"
    local sname=${4:-last_scrub_stamp}

    for ((i=0; i < $TIMEOUT; i++)); do
        sleep 0.2
        if test "$(get_last_scrub_stamp $pgid $sname)" '>' "$last_scrub" ; then
            return 0
        fi
        sleep 1
        # are we still the primary?
        local current_primary=`bin/stone pg $pgid query | jq '.acting[0]' `
        if [ $orig_primary != $current_primary ]; then
            echo $orig_primary no longer primary for $pgid
            return 0
        fi
    done
    return 1
}

##
# A modified version of pg_scrub()
#
# Run scrub on **pgid** and wait until it completes. The pg_scrub
# function will fail if repair does not complete within $TIMEOUT
# seconds. The pg_scrub is complete whenever the
# **get_last_scrub_stamp** function reports a timestamp different from
# the one stored before starting the scrub, or whenever the Primary
# changes.
#
# @param pgid the id of the PG
# @return 0 on success, 1 on error
#
function pg_scrub_mod() {
    local pgid=$1
    local last_scrub=$(get_last_scrub_stamp $pgid)
    # locate the primary
    local my_primary=`bin/stone pg $pgid query | jq '.acting[0]' `
    stone pg scrub $pgid
    wait_for_scrub_mod $pgid $my_primary "$last_scrub"
}

# update a map of 'log filename' -> 'current line count'
#
# @param (pos. 1) logfiles directory
# @param (pos. 2) the map to update. An associative array of starting line
#                 numbers (indexed by filename)
function mark_logs_linecount() {
    local odir=$1
    local -n wca=$2
    for f in  $odir/osd.*.log ;
    do
	local W=`wc -l $f | gawk '  {print($1);}' `
	wca["$f"]=$W
    done
}

##
# search a (log) file for a string, starting the search from a specific line
#
# @param (pos. 1) associative array of starting line numbers (indexed by filename)
# @param (pos. 2) the file to search
# @param (pos. 3) the text string to search for
# @returns 0 if found
function grep_log_after_linecount() {
    local -n lwca=$1
    local logfile=$2
    local from_line=${lwca[$logfile]}
    local text=$3
    from_line=`expr $from_line + 1`

    tail --lines=+$from_line $logfile | grep -q -e $text
    return $?
}

##
# search all osd logs for a string, starting the search from a specific line
#
# @param (pos. 1) logfiles directory
# @param (pos. 2) associative array of starting line numbers (indexed by filename)
# @param (pos. 3) the text string to search for
# @returns 0 if found in any of the files
function grep_all_after_linecount() {
    local dir=$1
    local -n wca=$2
    local text=$3

    for osd in $(seq 0 $(expr $OSDS - 1))
    do
        grep_log_after_linecount wca $dir/osd.$osd.log $text  && return 0
    done
    return 1
}

# osd_scrub_during_recovery=true make sure scrub happens
function TEST_recovery_scrub_2() {
    local dir=$1
    local poolname=test
    declare -A logwc # an associative array: log -> line number

    TESTDATA="testdata.$$"
    OSDS=8
    PGS=32
    OBJECTS=4

    setup $dir || return 1
    run_mon $dir a --osd_pool_default_size=1 --mon_allow_pool_size_one=true \
                   --osd_scrub_interval_randomize_ratio=0.0 || return 1
    run_mgr $dir x || return 1
    for osd in $(seq 0 $(expr $OSDS - 1))
    do
        run_osd $dir $osd --osd_scrub_during_recovery=true || return 1
    done

    # Create a pool with $PGS pgs
    create_pool $poolname $PGS $PGS
    wait_for_clean || return 1
    poolid=$(stone osd dump | grep "^pool.*[']test[']" | awk '{ print $2 }')

    dd if=/dev/urandom of=$TESTDATA bs=1M count=50
    for i in $(seq 1 $OBJECTS)
    do
        rados -p $poolname put obj${i} $TESTDATA
    done
    rm -f $TESTDATA

    flush_pg_stats
    date  --rfc-3339=ns
    mark_logs_linecount $dir logwc
    stone osd pool set $poolname size 3

    stone pg dump pgs

    # Wait for recovery to start
    set -o pipefail
    count=0
    while(true)
    do
      grep_all_after_linecount $dir logwc recovering && break
      if stone --format json pg dump pgs |
        jq '.pg_stats | [.[] | .state | contains("recovering")]' | grep -q true
      then
        break
      fi
      flush_pg_stats
      sleep 2
      if test "$count" -eq "10"
      then
        echo "Recovery never started"
        return 1
      fi
      count=$(expr $count + 1)
    done
    flush_pg_stats
    sleep 2
    set +o pipefail
    stone pg dump pgs

    pids=""
    for pg in $(seq 0 $(expr $PGS - 1))
    do
        run_in_background pids pg_scrub_mod $poolid.$(printf "%x" $pg)
    done
    stone pg dump pgs
    wait_background pids
    return_code=$?
    if [ $return_code -ne 0 ]; then return $return_code; fi

    ERRORS=0
    pidfile=$(find $dir 2>/dev/null | grep $name_prefix'[^/]*\.pid')
    pid=$(cat $pidfile)
    if ! kill -0 $pid
    then
        echo "OSD crash occurred"
        #tail -100 $dir/osd.0.log
        ERRORS=$(expr $ERRORS + 1)
    fi

    # Work around for http://tracker.stone.com/issues/38195
    kill_daemons $dir #|| return 1

    declare -a err_strings
    err_strings[0]="not scheduling scrubs due to active recovery"

    for osd in $(seq 0 $(expr $OSDS - 1))
    do
        grep "not scheduling scrubs" $dir/osd.${osd}.log
    done
    for err_string in "${err_strings[@]}"
    do
        found=false
        for osd in $(seq 0 $(expr $OSDS - 1))
        do
            if grep "$err_string" $dir/osd.${osd}.log > /dev/null;
            then
                found=true
            fi
        done
        if [ "$found" = "true" ]; then
            echo "Found log message not expected '$err_string'"
	    ERRORS=$(expr $ERRORS + 1)
        fi
    done

    teardown $dir || return 1

    if [ $ERRORS != "0" ];
    then
        echo "TEST FAILED WITH $ERRORS ERRORS"
        return 1
    fi

    echo "TEST PASSED"
    return 0
}

main osd-recovery-scrub "$@"

# Local Variables:
# compile-command: "cd build ; make -j4 && \
#    ../qa/run-standalone.sh osd-recovery-scrub.sh"
# End:
