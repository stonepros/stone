#!/usr/bin/env bash
# -*- mode:shell-script; tab-width:8; sh-basic-offset:2; indent-tabs-mode:t -*-
# vim: ts=8 sw=8 ft=bash smarttab
set -x

source $(dirname $0)/../../standalone/stone-helpers.sh

set -e
set -o functrace
PS4='${BASH_SOURCE[0]}:$LINENO: ${FUNCNAME[0]}:  '
SUDO=${SUDO:-sudo}
export STONE_DEV=1

function check_no_osd_down()
{
    ! stone osd dump | grep ' down '
}

function wait_no_osd_down()
{
  max_run=300
  for i in $(seq 1 $max_run) ; do
    if ! check_no_osd_down ; then
      echo "waiting for osd(s) to come back up ($i/$max_run)"
      sleep 1
    else
      break
    fi
  done
  check_no_osd_down
}

function expect_false()
{
	set -x
	if "$@"; then return 1; else return 0; fi
}

function expect_true()
{
	set -x
	if ! "$@"; then return 1; else return 0; fi
}

TEMP_DIR=$(mktemp -d ${TMPDIR-/tmp}/stonetool.XXX)
trap "rm -fr $TEMP_DIR" 0

TMPFILE=$(mktemp $TEMP_DIR/test_invalid.XXX)

#
# retry_eagain max cmd args ...
#
# retry cmd args ... if it exits on error and its output contains the
# string EAGAIN, at most $max times
#
function retry_eagain()
{
    local max=$1
    shift
    local status
    local tmpfile=$TEMP_DIR/retry_eagain.$$
    local count
    for count in $(seq 1 $max) ; do
        status=0
        "$@" > $tmpfile 2>&1 || status=$?
        if test $status = 0 || 
            ! grep --quiet EAGAIN $tmpfile ; then
            break
        fi
        sleep 1
    done
    if test $count = $max ; then
        echo retried with non zero exit status, $max times: "$@" >&2
    fi
    cat $tmpfile
    rm $tmpfile
    return $status
}

#
# map_enxio_to_eagain cmd arg ...
#
# add EAGAIN to the output of cmd arg ... if the output contains
# ENXIO.
#
function map_enxio_to_eagain()
{
    local status=0
    local tmpfile=$TEMP_DIR/map_enxio_to_eagain.$$

    "$@" > $tmpfile 2>&1 || status=$?
    if test $status != 0 &&
        grep --quiet ENXIO $tmpfile ; then
        echo "EAGAIN added by $0::map_enxio_to_eagain" >> $tmpfile
    fi
    cat $tmpfile
    rm $tmpfile
    return $status
}

function check_response()
{
	expected_string=$1
	retcode=$2
	expected_retcode=$3
	if [ "$expected_retcode" -a $retcode != $expected_retcode ] ; then
		echo "return code invalid: got $retcode, expected $expected_retcode" >&2
		exit 1
	fi

	if ! grep --quiet -- "$expected_string" $TMPFILE ; then 
		echo "Didn't find $expected_string in output" >&2
		cat $TMPFILE >&2
		exit 1
	fi
}

function get_config_value_or_die()
{
  local target config_opt raw val

  target=$1
  config_opt=$2

  raw="`$SUDO stone daemon $target config get $config_opt 2>/dev/null`"
  if [[ $? -ne 0 ]]; then
    echo "error obtaining config opt '$config_opt' from '$target': $raw"
    exit 1
  fi

  raw=`echo $raw | sed -e 's/[{} "]//g'`
  val=`echo $raw | cut -f2 -d:`

  echo "$val"
  return 0
}

function expect_config_value()
{
  local target config_opt expected_val val
  target=$1
  config_opt=$2
  expected_val=$3

  val=$(get_config_value_or_die $target $config_opt)

  if [[ "$val" != "$expected_val" ]]; then
    echo "expected '$expected_val', got '$val'"
    exit 1
  fi
}

function stone_watch_start()
{
    local whatch_opt=--watch

    if [ -n "$1" ]; then
	whatch_opt=--watch-$1
	if [ -n "$2" ]; then
	    whatch_opt+=" --watch-channel $2"
	fi
    fi

    STONE_WATCH_FILE=${TEMP_DIR}/STONE_WATCH_$$
    stone $whatch_opt > $STONE_WATCH_FILE &
    STONE_WATCH_PID=$!

    # wait until the "stone" client is connected and receiving
    # log messages from monitor
    for i in `seq 3`; do
        grep -q "cluster" $STONE_WATCH_FILE && break
        sleep 1
    done
}

function stone_watch_wait()
{
    local regexp=$1
    local timeout=30

    if [ -n "$2" ]; then
	timeout=$2
    fi

    for i in `seq ${timeout}`; do
	grep -q "$regexp" $STONE_WATCH_FILE && break
	sleep 1
    done

    kill $STONE_WATCH_PID

    if ! grep "$regexp" $STONE_WATCH_FILE; then
	echo "pattern ${regexp} not found in watch file. Full watch file content:" >&2
	cat $STONE_WATCH_FILE >&2
	return 1
    fi
}

function test_mon_injectargs()
{
  stone tell osd.0 injectargs --no-osd_enable_op_tracker
  stone tell osd.0 config get osd_enable_op_tracker | grep false
  stone tell osd.0 injectargs '--osd_enable_op_tracker --osd_op_history_duration 500'
  stone tell osd.0 config get osd_enable_op_tracker | grep true
  stone tell osd.0 config get osd_op_history_duration | grep 500
  stone tell osd.0 injectargs --no-osd_enable_op_tracker
  stone tell osd.0 config get osd_enable_op_tracker | grep false
  stone tell osd.0 injectargs -- --osd_enable_op_tracker
  stone tell osd.0 config get osd_enable_op_tracker | grep true
  stone tell osd.0 injectargs -- '--osd_enable_op_tracker --osd_op_history_duration 600'
  stone tell osd.0 config get osd_enable_op_tracker | grep true
  stone tell osd.0 config get osd_op_history_duration | grep 600

  stone tell osd.0 injectargs -- '--osd_deep_scrub_interval 2419200'
  stone tell osd.0 config get osd_deep_scrub_interval | grep 2419200

  stone tell osd.0 injectargs -- '--mon_probe_timeout 2'
  stone tell osd.0 config get mon_probe_timeout | grep 2

  stone tell osd.0 injectargs -- '--mon-lease 6'
  stone tell osd.0 config get mon_lease | grep 6

  # osd-scrub-auto-repair-num-errors is an OPT_U32, so -1 is not a valid setting
  expect_false stone tell osd.0 injectargs --osd-scrub-auto-repair-num-errors -1  2> $TMPFILE || return 1
  check_response "Error EINVAL: Parse error setting osd_scrub_auto_repair_num_errors to '-1' using injectargs"

  expect_failure $TEMP_DIR "Option --osd_op_history_duration requires an argument" \
                 stone tell osd.0 injectargs -- '--osd_op_history_duration'

}

function test_mon_injectargs_SI()
{
  # Test SI units during injectargs and 'config set'
  # We only aim at testing the units are parsed accordingly
  # and don't intend to test whether the options being set
  # actually expect SI units to be passed.
  # Keep in mind that all integer based options that are not based on bytes
  # (i.e., INT, LONG, U32, U64) will accept SI unit modifiers and be parsed to
  # base 10.
  initial_value=$(get_config_value_or_die "mon.a" "mon_pg_warn_min_objects")
  $SUDO stone daemon mon.a config set mon_pg_warn_min_objects 10
  expect_config_value "mon.a" "mon_pg_warn_min_objects" 10
  $SUDO stone daemon mon.a config set mon_pg_warn_min_objects 10K
  expect_config_value "mon.a" "mon_pg_warn_min_objects" 10000
  $SUDO stone daemon mon.a config set mon_pg_warn_min_objects 1G
  expect_config_value "mon.a" "mon_pg_warn_min_objects" 1000000000
  $SUDO stone daemon mon.a config set mon_pg_warn_min_objects 10F > $TMPFILE || true
  check_response "(22) Invalid argument"
  # now test with injectargs
  stone tell mon.a injectargs '--mon_pg_warn_min_objects 10'
  expect_config_value "mon.a" "mon_pg_warn_min_objects" 10
  stone tell mon.a injectargs '--mon_pg_warn_min_objects 10K'
  expect_config_value "mon.a" "mon_pg_warn_min_objects" 10000
  stone tell mon.a injectargs '--mon_pg_warn_min_objects 1G'
  expect_config_value "mon.a" "mon_pg_warn_min_objects" 1000000000
  expect_false stone tell mon.a injectargs '--mon_pg_warn_min_objects 10F'
  expect_false stone tell mon.a injectargs '--mon_globalid_prealloc -1'
  $SUDO stone daemon mon.a config set mon_pg_warn_min_objects $initial_value
}

function test_mon_injectargs_IEC()
{
  # Test IEC units during injectargs and 'config set'
  # We only aim at testing the units are parsed accordingly
  # and don't intend to test whether the options being set
  # actually expect IEC units to be passed.
  # Keep in mind that all integer based options that are based on bytes
  # (i.e., INT, LONG, U32, U64) will accept IEC unit modifiers, as well as SI
  # unit modifiers (for backwards compatibility and convenience) and be parsed
  # to base 2.
  initial_value=$(get_config_value_or_die "mon.a" "mon_data_size_warn")
  $SUDO stone daemon mon.a config set mon_data_size_warn 15000000000
  expect_config_value "mon.a" "mon_data_size_warn" 15000000000
  $SUDO stone daemon mon.a config set mon_data_size_warn 15G
  expect_config_value "mon.a" "mon_data_size_warn" 16106127360
  $SUDO stone daemon mon.a config set mon_data_size_warn 16Gi
  expect_config_value "mon.a" "mon_data_size_warn" 17179869184
  $SUDO stone daemon mon.a config set mon_data_size_warn 10F > $TMPFILE || true
  check_response "(22) Invalid argument"
  # now test with injectargs
  stone tell mon.a injectargs '--mon_data_size_warn 15000000000'
  expect_config_value "mon.a" "mon_data_size_warn" 15000000000
  stone tell mon.a injectargs '--mon_data_size_warn 15G'
  expect_config_value "mon.a" "mon_data_size_warn" 16106127360
  stone tell mon.a injectargs '--mon_data_size_warn 16Gi'
  expect_config_value "mon.a" "mon_data_size_warn" 17179869184
  expect_false stone tell mon.a injectargs '--mon_data_size_warn 10F'
  $SUDO stone daemon mon.a config set mon_data_size_warn $initial_value
}

function test_tiering_agent()
{
  local slow=slow_eviction
  local fast=fast_eviction
  stone osd pool create $slow  1 1
  stone osd pool application enable $slow rados
  stone osd pool create $fast  1 1
  stone osd tier add $slow $fast
  stone osd tier cache-mode $fast writeback
  stone osd tier set-overlay $slow $fast
  stone osd pool set $fast hit_set_type bloom
  rados -p $slow put obj1 /etc/group
  stone osd pool set $fast target_max_objects  1
  stone osd pool set $fast hit_set_count 1
  stone osd pool set $fast hit_set_period 5
  # wait for the object to be evicted from the cache
  local evicted
  evicted=false
  for i in `seq 1 300` ; do
      if ! rados -p $fast ls | grep obj1 ; then
          evicted=true
          break
      fi
      sleep 1
  done
  $evicted # assert
  # the object is proxy read and promoted to the cache
  rados -p $slow get obj1 - >/dev/null
  # wait for the promoted object to be evicted again
  evicted=false
  for i in `seq 1 300` ; do
      if ! rados -p $fast ls | grep obj1 ; then
          evicted=true
          break
      fi
      sleep 1
  done
  $evicted # assert
  stone osd tier remove-overlay $slow
  stone osd tier remove $slow $fast
  stone osd pool delete $fast $fast --yes-i-really-really-mean-it
  stone osd pool delete $slow $slow --yes-i-really-really-mean-it
}

function test_tiering_1()
{
  # tiering
  stone osd pool create slow 2
  stone osd pool application enable slow rados
  stone osd pool create slow2 2
  stone osd pool application enable slow2 rados
  stone osd pool create cache 2
  stone osd pool create cache2 2
  stone osd tier add slow cache
  stone osd tier add slow cache2
  expect_false stone osd tier add slow2 cache
  # application metadata should propagate to the tiers
  stone osd pool ls detail -f json | jq '.[] | select(.pool_name == "slow") | .application_metadata["rados"]' | grep '{}'
  stone osd pool ls detail -f json | jq '.[] | select(.pool_name == "slow2") | .application_metadata["rados"]' | grep '{}'
  stone osd pool ls detail -f json | jq '.[] | select(.pool_name == "cache") | .application_metadata["rados"]' | grep '{}'
  stone osd pool ls detail -f json | jq '.[] | select(.pool_name == "cache2") | .application_metadata["rados"]' | grep '{}'
  # forward and proxy are removed/deprecated
  expect_false stone osd tier cache-mode cache forward
  expect_false stone osd tier cache-mode cache forward --yes-i-really-mean-it
  expect_false stone osd tier cache-mode cache proxy
  expect_false stone osd tier cache-mode cache proxy --yes-i-really-mean-it
  # test some state transitions
  stone osd tier cache-mode cache writeback
  expect_false stone osd tier cache-mode cache readonly
  expect_false stone osd tier cache-mode cache readonly --yes-i-really-mean-it
  stone osd tier cache-mode cache readproxy
  stone osd tier cache-mode cache none
  stone osd tier cache-mode cache readonly --yes-i-really-mean-it
  stone osd tier cache-mode cache none
  stone osd tier cache-mode cache writeback
  expect_false stone osd tier cache-mode cache none
  expect_false stone osd tier cache-mode cache readonly --yes-i-really-mean-it
  # test with dirty objects in the tier pool
  # tier pool currently set to 'writeback'
  rados -p cache put /etc/passwd /etc/passwd
  flush_pg_stats
  # 1 dirty object in pool 'cache'
  stone osd tier cache-mode cache readproxy
  expect_false stone osd tier cache-mode cache none
  expect_false stone osd tier cache-mode cache readonly --yes-i-really-mean-it
  stone osd tier cache-mode cache writeback
  # remove object from tier pool
  rados -p cache rm /etc/passwd
  rados -p cache cache-flush-evict-all
  flush_pg_stats
  # no dirty objects in pool 'cache'
  stone osd tier cache-mode cache readproxy
  stone osd tier cache-mode cache none
  stone osd tier cache-mode cache readonly --yes-i-really-mean-it
  TRIES=0
  while ! stone osd pool set cache pg_num 3 --yes-i-really-mean-it 2>$TMPFILE
  do
    grep 'currently creating pgs' $TMPFILE
    TRIES=$(( $TRIES + 1 ))
    test $TRIES -ne 60
    sleep 3
  done
  expect_false stone osd pool set cache pg_num 4
  stone osd tier cache-mode cache none
  stone osd tier set-overlay slow cache
  expect_false stone osd tier set-overlay slow cache2
  expect_false stone osd tier remove slow cache
  stone osd tier remove-overlay slow
  stone osd tier set-overlay slow cache2
  stone osd tier remove-overlay slow
  stone osd tier remove slow cache
  stone osd tier add slow2 cache
  expect_false stone osd tier set-overlay slow cache
  stone osd tier set-overlay slow2 cache
  stone osd tier remove-overlay slow2
  stone osd tier remove slow2 cache
  stone osd tier remove slow cache2

  # make sure a non-empty pool fails
  rados -p cache2 put /etc/passwd /etc/passwd
  while ! stone df | grep cache2 | grep ' 1 ' ; do
    echo waiting for pg stats to flush
    sleep 2
  done
  expect_false stone osd tier add slow cache2
  stone osd tier add slow cache2 --force-nonempty
  stone osd tier remove slow cache2

  stone osd pool ls | grep cache2
  stone osd pool ls -f json-pretty | grep cache2
  stone osd pool ls detail | grep cache2
  stone osd pool ls detail -f json-pretty | grep cache2

  stone osd pool delete slow slow --yes-i-really-really-mean-it
  stone osd pool delete slow2 slow2 --yes-i-really-really-mean-it
  stone osd pool delete cache cache --yes-i-really-really-mean-it
  stone osd pool delete cache2 cache2 --yes-i-really-really-mean-it
}

function test_tiering_2()
{
  # make sure we can't clobber snapshot state
  stone osd pool create snap_base 2
  stone osd pool application enable snap_base rados
  stone osd pool create snap_cache 2
  stone osd pool mksnap snap_cache snapname
  expect_false stone osd tier add snap_base snap_cache
  stone osd pool delete snap_base snap_base --yes-i-really-really-mean-it
  stone osd pool delete snap_cache snap_cache --yes-i-really-really-mean-it
}

function test_tiering_3()
{
  # make sure we can't create snapshot on tier
  stone osd pool create basex 2
  stone osd pool application enable basex rados
  stone osd pool create cachex 2
  stone osd tier add basex cachex
  expect_false stone osd pool mksnap cache snapname
  stone osd tier remove basex cachex
  stone osd pool delete basex basex --yes-i-really-really-mean-it
  stone osd pool delete cachex cachex --yes-i-really-really-mean-it
}

function test_tiering_4()
{
  # make sure we can't create an ec pool tier
  stone osd pool create eccache 2 2 erasure
  expect_false stone osd set-require-min-compat-client bobtail
  stone osd pool create repbase 2
  stone osd pool application enable repbase rados
  expect_false stone osd tier add repbase eccache
  stone osd pool delete repbase repbase --yes-i-really-really-mean-it
  stone osd pool delete eccache eccache --yes-i-really-really-mean-it
}

function test_tiering_5()
{
  # convenient add-cache command
  stone osd pool create slow 2
  stone osd pool application enable slow rados
  stone osd pool create cache3 2
  stone osd tier add-cache slow cache3 1024000
  stone osd dump | grep cache3 | grep bloom | grep 'false_positive_probability: 0.05' | grep 'target_bytes 1024000' | grep '1200s x4'
  stone osd tier remove slow cache3 2> $TMPFILE || true
  check_response "EBUSY: tier pool 'cache3' is the overlay for 'slow'; please remove-overlay first"
  stone osd tier remove-overlay slow
  stone osd tier remove slow cache3
  stone osd pool ls | grep cache3
  stone osd pool delete cache3 cache3 --yes-i-really-really-mean-it
  ! stone osd pool ls | grep cache3 || exit 1
  stone osd pool delete slow slow --yes-i-really-really-mean-it
}

function test_tiering_6()
{
  # check add-cache whether work
  stone osd pool create datapool 2
  stone osd pool application enable datapool rados
  stone osd pool create cachepool 2
  stone osd tier add-cache datapool cachepool 1024000
  stone osd tier cache-mode cachepool writeback
  rados -p datapool put object /etc/passwd
  rados -p cachepool stat object
  rados -p cachepool cache-flush object
  rados -p datapool stat object
  stone osd tier remove-overlay datapool
  stone osd tier remove datapool cachepool
  stone osd pool delete cachepool cachepool --yes-i-really-really-mean-it
  stone osd pool delete datapool datapool --yes-i-really-really-mean-it
}

function test_tiering_7()
{
  # protection against pool removal when used as tiers
  stone osd pool create datapool 2
  stone osd pool application enable datapool rados
  stone osd pool create cachepool 2
  stone osd tier add-cache datapool cachepool 1024000
  stone osd pool delete cachepool cachepool --yes-i-really-really-mean-it 2> $TMPFILE || true
  check_response "EBUSY: pool 'cachepool' is a tier of 'datapool'"
  stone osd pool delete datapool datapool --yes-i-really-really-mean-it 2> $TMPFILE || true
  check_response "EBUSY: pool 'datapool' has tiers cachepool"
  stone osd tier remove-overlay datapool
  stone osd tier remove datapool cachepool
  stone osd pool delete cachepool cachepool --yes-i-really-really-mean-it
  stone osd pool delete datapool datapool --yes-i-really-really-mean-it
}

function test_tiering_8()
{
  ## check health check
  stone osd set notieragent
  stone osd pool create datapool 2
  stone osd pool application enable datapool rados
  stone osd pool create cache4 2
  stone osd tier add-cache datapool cache4 1024000
  stone osd tier cache-mode cache4 writeback
  tmpfile=$(mktemp|grep tmp)
  dd if=/dev/zero of=$tmpfile  bs=4K count=1
  stone osd pool set cache4 target_max_objects 200
  stone osd pool set cache4 target_max_bytes 1000000
  rados -p cache4 put foo1 $tmpfile
  rados -p cache4 put foo2 $tmpfile
  rm -f $tmpfile
  flush_pg_stats
  stone df | grep datapool | grep ' 2 '
  stone osd tier remove-overlay datapool
  stone osd tier remove datapool cache4
  stone osd pool delete cache4 cache4 --yes-i-really-really-mean-it
  stone osd pool delete datapool datapool --yes-i-really-really-mean-it
  stone osd unset notieragent
}

function test_tiering_9()
{
  # make sure 'tier remove' behaves as we expect
  # i.e., removing a tier from a pool that's not its base pool only
  # results in a 'pool foo is now (or already was) not a tier of bar'
  #
  stone osd pool create basepoolA 2
  stone osd pool application enable basepoolA rados
  stone osd pool create basepoolB 2
  stone osd pool application enable basepoolB rados
  poolA_id=$(stone osd dump | grep 'pool.*basepoolA' | awk '{print $2;}')
  poolB_id=$(stone osd dump | grep 'pool.*basepoolB' | awk '{print $2;}')

  stone osd pool create cache5 2
  stone osd pool create cache6 2
  stone osd tier add basepoolA cache5
  stone osd tier add basepoolB cache6
  stone osd tier remove basepoolB cache5 2>&1 | grep 'not a tier of'
  stone osd dump | grep "pool.*'cache5'" 2>&1 | grep "tier_of[ \t]\+$poolA_id"
  stone osd tier remove basepoolA cache6 2>&1 | grep 'not a tier of'
  stone osd dump | grep "pool.*'cache6'" 2>&1 | grep "tier_of[ \t]\+$poolB_id"

  stone osd tier remove basepoolA cache5 2>&1 | grep 'not a tier of'
  ! stone osd dump | grep "pool.*'cache5'" 2>&1 | grep "tier_of" || exit 1
  stone osd tier remove basepoolB cache6 2>&1 | grep 'not a tier of'
  ! stone osd dump | grep "pool.*'cache6'" 2>&1 | grep "tier_of" || exit 1

  ! stone osd dump | grep "pool.*'basepoolA'" 2>&1 | grep "tiers" || exit 1
  ! stone osd dump | grep "pool.*'basepoolB'" 2>&1 | grep "tiers" || exit 1

  stone osd pool delete cache6 cache6 --yes-i-really-really-mean-it
  stone osd pool delete cache5 cache5 --yes-i-really-really-mean-it
  stone osd pool delete basepoolB basepoolB --yes-i-really-really-mean-it
  stone osd pool delete basepoolA basepoolA --yes-i-really-really-mean-it
}

function test_auth()
{
  expect_false stone auth add client.xx mon 'invalid' osd "allow *"
  expect_false stone auth add client.xx mon 'allow *' osd "allow *" invalid "allow *"
  stone auth add client.xx mon 'allow *' osd "allow *"
  stone auth export client.xx >client.xx.keyring
  stone auth add client.xx -i client.xx.keyring
  rm -f client.xx.keyring
  stone auth list | grep client.xx
  stone auth ls | grep client.xx
  stone auth get client.xx | grep caps | grep mon
  stone auth get client.xx | grep caps | grep osd
  stone auth get-key client.xx
  stone auth print-key client.xx
  stone auth print_key client.xx
  stone auth caps client.xx osd "allow rw"
  expect_false sh <<< "stone auth get client.xx | grep caps | grep mon"
  stone auth get client.xx | grep osd | grep "allow rw"
  stone auth caps client.xx mon 'allow command "osd tree"'
  stone auth export | grep client.xx
  stone auth export -o authfile
  stone auth import -i authfile 2>$TMPFILE
  check_response "imported keyring"

  stone auth export -o authfile2
  diff authfile authfile2
  rm authfile authfile2
  stone auth del client.xx
  expect_false stone auth get client.xx

  # (almost) interactive mode
  echo -e 'auth add client.xx mon "allow *" osd "allow *"\n' | stone
  stone auth get client.xx
  # script mode
  echo 'auth del client.xx' | stone
  expect_false stone auth get client.xx
}

function test_auth_profiles()
{
  stone auth add client.xx-profile-ro mon 'allow profile read-only' \
       mgr 'allow profile read-only'
  stone auth add client.xx-profile-rw mon 'allow profile read-write' \
       mgr 'allow profile read-write'
  stone auth add client.xx-profile-rd mon 'allow profile role-definer'

  stone auth export > client.xx.keyring

  # read-only is allowed all read-only commands (auth excluded)
  stone -n client.xx-profile-ro -k client.xx.keyring status
  stone -n client.xx-profile-ro -k client.xx.keyring osd dump
  stone -n client.xx-profile-ro -k client.xx.keyring pg dump
  stone -n client.xx-profile-ro -k client.xx.keyring mon dump
  # read-only gets access denied for rw commands or auth commands
  stone -n client.xx-profile-ro -k client.xx.keyring log foo >& $TMPFILE || true
  check_response "EACCES: access denied"
  stone -n client.xx-profile-ro -k client.xx.keyring osd set noout >& $TMPFILE || true
  check_response "EACCES: access denied"
  stone -n client.xx-profile-ro -k client.xx.keyring auth ls >& $TMPFILE || true
  check_response "EACCES: access denied"

  # read-write is allowed for all read-write commands (except auth)
  stone -n client.xx-profile-rw -k client.xx.keyring status
  stone -n client.xx-profile-rw -k client.xx.keyring osd dump
  stone -n client.xx-profile-rw -k client.xx.keyring pg dump
  stone -n client.xx-profile-rw -k client.xx.keyring mon dump
  stone -n client.xx-profile-rw -k client.xx.keyring fs dump
  stone -n client.xx-profile-rw -k client.xx.keyring log foo
  stone -n client.xx-profile-rw -k client.xx.keyring osd set noout
  stone -n client.xx-profile-rw -k client.xx.keyring osd unset noout
  # read-write gets access denied for auth commands
  stone -n client.xx-profile-rw -k client.xx.keyring auth ls >& $TMPFILE || true
  check_response "EACCES: access denied"

  # role-definer is allowed RWX 'auth' commands and read-only 'mon' commands
  stone -n client.xx-profile-rd -k client.xx.keyring auth ls
  stone -n client.xx-profile-rd -k client.xx.keyring auth export
  stone -n client.xx-profile-rd -k client.xx.keyring auth add client.xx-profile-foo
  stone -n client.xx-profile-rd -k client.xx.keyring status
  stone -n client.xx-profile-rd -k client.xx.keyring osd dump >& $TMPFILE || true
  check_response "EACCES: access denied"
  stone -n client.xx-profile-rd -k client.xx.keyring pg dump >& $TMPFILE || true
  check_response "EACCES: access denied"
  # read-only 'mon' subsystem commands are allowed
  stone -n client.xx-profile-rd -k client.xx.keyring mon dump
  # but read-write 'mon' commands are not
  stone -n client.xx-profile-rd -k client.xx.keyring mon add foo 1.1.1.1 >& $TMPFILE || true
  check_response "EACCES: access denied"
  stone -n client.xx-profile-rd -k client.xx.keyring fs dump >& $TMPFILE || true
  check_response "EACCES: access denied"
  stone -n client.xx-profile-rd -k client.xx.keyring log foo >& $TMPFILE || true
  check_response "EACCES: access denied"
  stone -n client.xx-profile-rd -k client.xx.keyring osd set noout >& $TMPFILE || true
  check_response "EACCES: access denied"

  stone -n client.xx-profile-rd -k client.xx.keyring auth del client.xx-profile-ro
  stone -n client.xx-profile-rd -k client.xx.keyring auth del client.xx-profile-rw
  
  # add a new role-definer with the existing role-definer
  stone -n client.xx-profile-rd -k client.xx.keyring \
    auth add client.xx-profile-rd2 mon 'allow profile role-definer'
  stone -n client.xx-profile-rd -k client.xx.keyring \
    auth export > client.xx.keyring.2
  # remove old role-definer using the new role-definer
  stone -n client.xx-profile-rd2 -k client.xx.keyring.2 \
    auth del client.xx-profile-rd
  # remove the remaining role-definer with admin
  stone auth del client.xx-profile-rd2
  rm -f client.xx.keyring client.xx.keyring.2
}

function test_mon_caps()
{
  stone-authtool --create-keyring $TEMP_DIR/stone.client.bug.keyring
  chmod +r  $TEMP_DIR/stone.client.bug.keyring
  stone-authtool  $TEMP_DIR/stone.client.bug.keyring -n client.bug --gen-key
  stone auth add client.bug -i  $TEMP_DIR/stone.client.bug.keyring

  # pass --no-mon-config since we are looking for the permission denied error
  rados lspools --no-mon-config --keyring $TEMP_DIR/stone.client.bug.keyring -n client.bug >& $TMPFILE || true
  cat $TMPFILE
  check_response "Permission denied"

  rm -rf $TEMP_DIR/stone.client.bug.keyring
  stone auth del client.bug
  stone-authtool --create-keyring $TEMP_DIR/stone.client.bug.keyring
  chmod +r  $TEMP_DIR/stone.client.bug.keyring
  stone-authtool  $TEMP_DIR/stone.client.bug.keyring -n client.bug --gen-key
  stone-authtool -n client.bug --cap mon '' $TEMP_DIR/stone.client.bug.keyring
  stone auth add client.bug -i  $TEMP_DIR/stone.client.bug.keyring
  rados lspools --no-mon-config --keyring $TEMP_DIR/stone.client.bug.keyring -n client.bug >& $TMPFILE || true
  check_response "Permission denied"  
}

function test_mon_misc()
{
  # with and without verbosity
  stone osd dump | grep '^epoch'
  stone --concise osd dump | grep '^epoch'

  stone osd df | grep 'MIN/MAX VAR'

  # df
  stone df > $TMPFILE
  grep RAW $TMPFILE
  grep -v DIRTY $TMPFILE
  stone df detail > $TMPFILE
  grep DIRTY $TMPFILE
  stone df --format json > $TMPFILE
  grep 'total_bytes' $TMPFILE
  grep -v 'dirty' $TMPFILE
  stone df detail --format json > $TMPFILE
  grep 'rd_bytes' $TMPFILE
  grep 'dirty' $TMPFILE
  stone df --format xml | grep '<total_bytes>'
  stone df detail --format xml | grep '<rd_bytes>'

  stone fsid
  stone health
  stone health detail
  stone health --format json-pretty
  stone health detail --format xml-pretty

  stone time-sync-status

  stone node ls
  for t in mon osd mds mgr ; do
      stone node ls $t
  done

  stone_watch_start
  mymsg="this is a test log message $$.$(date)"
  stone log "$mymsg"
  stone log last | grep "$mymsg"
  stone log last 100 | grep "$mymsg"
  stone_watch_wait "$mymsg"

  stone mgr stat
  stone mgr dump
  stone mgr module ls
  stone mgr module enable restful
  expect_false stone mgr module enable foodne
  stone mgr module enable foodne --force
  stone mgr module disable foodne
  stone mgr module disable foodnebizbangbash

  stone mon metadata a
  stone mon metadata
  stone mon count-metadata stone_version
  stone mon versions

  stone mgr metadata
  stone mgr versions
  stone mgr count-metadata stone_version

  stone versions

  stone node ls
}

function check_mds_active()
{
    fs_name=$1
    stone fs get $fs_name | grep active
}

function wait_mds_active()
{
  fs_name=$1
  max_run=300
  for i in $(seq 1 $max_run) ; do
      if ! check_mds_active $fs_name ; then
          echo "waiting for an active MDS daemon ($i/$max_run)"
          sleep 5
      else
          break
      fi
  done
  check_mds_active $fs_name
}

function get_mds_gids()
{
    fs_name=$1
    stone fs get $fs_name --format=json | python3 -c "import json; import sys; print(' '.join([m['gid'].__str__() for m in json.load(sys.stdin)['mdsmap']['info'].values()]))"
}

function fail_all_mds()
{
  fs_name=$1
  stone fs set $fs_name cluster_down true
  mds_gids=$(get_mds_gids $fs_name)
  for mds_gid in $mds_gids ; do
      stone mds fail $mds_gid
  done
  if check_mds_active $fs_name ; then
      echo "An active MDS remains, something went wrong"
      stone fs get $fs_name
      exit -1
  fi

}

function remove_all_fs()
{
  existing_fs=$(stone fs ls --format=json | python3 -c "import json; import sys; print(' '.join([fs['name'] for fs in json.load(sys.stdin)]))")
  for fs_name in $existing_fs ; do
      echo "Removing fs ${fs_name}..."
      fail_all_mds $fs_name
      echo "Removing existing filesystem '${fs_name}'..."
      stone fs rm $fs_name --yes-i-really-mean-it
      echo "Removed '${fs_name}'."
  done
}

# So that tests requiring MDS can skip if one is not configured
# in the cluster at all
function mds_exists()
{
    stone auth ls | grep "^mds"
}

# some of the commands are just not idempotent.
function without_test_dup_command()
{
  if [ -z ${STONE_CLI_TEST_DUP_COMMAND+x} ]; then
    $@
  else
    local saved=${STONE_CLI_TEST_DUP_COMMAND}
    unset STONE_CLI_TEST_DUP_COMMAND
    $@
    STONE_CLI_TEST_DUP_COMMAND=saved
  fi
}

function test_mds_tell()
{
  local FS_NAME=stonefs
  if ! mds_exists ; then
      echo "Skipping test, no MDS found"
      return
  fi

  remove_all_fs
  stone osd pool create fs_data 16
  stone osd pool create fs_metadata 16
  stone fs new $FS_NAME fs_metadata fs_data
  wait_mds_active $FS_NAME

  # Test injectargs by GID
  old_mds_gids=$(get_mds_gids $FS_NAME)
  echo Old GIDs: $old_mds_gids

  for mds_gid in $old_mds_gids ; do
      stone tell mds.$mds_gid injectargs "--debug-mds 20"
  done
  expect_false stone tell mds.a injectargs mds_max_file_recover -1

  # Test respawn by rank
  without_test_dup_command stone tell mds.0 respawn
  new_mds_gids=$old_mds_gids
  while [ $new_mds_gids -eq $old_mds_gids ] ; do
      sleep 5
      new_mds_gids=$(get_mds_gids $FS_NAME)
  done
  echo New GIDs: $new_mds_gids

  # Test respawn by ID
  without_test_dup_command stone tell mds.a respawn
  new_mds_gids=$old_mds_gids
  while [ $new_mds_gids -eq $old_mds_gids ] ; do
      sleep 5
      new_mds_gids=$(get_mds_gids $FS_NAME)
  done
  echo New GIDs: $new_mds_gids

  remove_all_fs
  stone osd pool delete fs_data fs_data --yes-i-really-really-mean-it
  stone osd pool delete fs_metadata fs_metadata --yes-i-really-really-mean-it
}

function test_mon_mds()
{
  local FS_NAME=stonefs
  remove_all_fs

  stone osd pool create fs_data 16
  stone osd pool create fs_metadata 16
  stone fs new $FS_NAME fs_metadata fs_data

  stone fs set $FS_NAME cluster_down true
  stone fs set $FS_NAME cluster_down false

  stone mds compat rm_incompat 4
  stone mds compat rm_incompat 4

  # We don't want any MDSs to be up, their activity can interfere with
  # the "current_epoch + 1" checking below if they're generating updates
  fail_all_mds $FS_NAME

  stone mds compat show
  stone fs dump
  stone fs get $FS_NAME
  for mds_gid in $(get_mds_gids $FS_NAME) ; do
      stone mds metadata $mds_id
  done
  stone mds metadata
  stone mds versions
  stone mds count-metadata os

  # XXX mds fail, but how do you undo it?
  mdsmapfile=$TEMP_DIR/mdsmap.$$
  current_epoch=$(stone fs dump -o $mdsmapfile --no-log-to-stderr 2>&1 | grep epoch | sed 's/.*epoch //')
  [ -s $mdsmapfile ]
  rm $mdsmapfile

  stone osd pool create data2 16
  stone osd pool create data3 16
  data2_pool=$(stone osd dump | grep "pool.*'data2'" | awk '{print $2;}')
  data3_pool=$(stone osd dump | grep "pool.*'data3'" | awk '{print $2;}')
  stone fs add_data_pool stonefs $data2_pool
  stone fs add_data_pool stonefs $data3_pool
  stone fs add_data_pool stonefs 100 >& $TMPFILE || true
  check_response "Error ENOENT"
  stone fs add_data_pool stonefs foobarbaz >& $TMPFILE || true
  check_response "Error ENOENT"
  stone fs rm_data_pool stonefs $data2_pool
  stone fs rm_data_pool stonefs $data3_pool
  stone osd pool delete data2 data2 --yes-i-really-really-mean-it
  stone osd pool delete data3 data3 --yes-i-really-really-mean-it
  stone fs set stonefs max_mds 4
  stone fs set stonefs max_mds 3
  stone fs set stonefs max_mds 256
  expect_false stone fs set stonefs max_mds 257
  stone fs set stonefs max_mds 4
  stone fs set stonefs max_mds 256
  expect_false stone fs set stonefs max_mds 257
  expect_false stone fs set stonefs max_mds asdf
  expect_false stone fs set stonefs inline_data true
  stone fs set stonefs inline_data true --yes-i-really-really-mean-it
  stone fs set stonefs inline_data yes --yes-i-really-really-mean-it
  stone fs set stonefs inline_data 1 --yes-i-really-really-mean-it
  expect_false stone fs set stonefs inline_data --yes-i-really-really-mean-it
  stone fs set stonefs inline_data false
  stone fs set stonefs inline_data no
  stone fs set stonefs inline_data 0
  expect_false stone fs set stonefs inline_data asdf
  stone fs set stonefs max_file_size 1048576
  expect_false stone fs set stonefs max_file_size 123asdf

  expect_false stone fs set stonefs allow_new_snaps
  stone fs set stonefs allow_new_snaps true
  stone fs set stonefs allow_new_snaps 0
  stone fs set stonefs allow_new_snaps false
  stone fs set stonefs allow_new_snaps no
  expect_false stone fs set stonefs allow_new_snaps taco

  # we should never be able to add EC pools as data or metadata pools
  # create an ec-pool...
  stone osd pool create mds-ec-pool 16 16 erasure
  set +e
  stone fs add_data_pool stonefs mds-ec-pool 2>$TMPFILE
  check_response 'erasure-code' $? 22
  set -e
  ec_poolnum=$(stone osd dump | grep "pool.* 'mds-ec-pool" | awk '{print $2;}')
  data_poolnum=$(stone osd dump | grep "pool.* 'fs_data" | awk '{print $2;}')
  metadata_poolnum=$(stone osd dump | grep "pool.* 'fs_metadata" | awk '{print $2;}')

  fail_all_mds $FS_NAME

  set +e
  # Check that rmfailed requires confirmation
  expect_false stone mds rmfailed 0
  stone mds rmfailed 0 --yes-i-really-mean-it
  set -e

  # Check that `fs new` is no longer permitted
  expect_false stone fs new stonefs $metadata_poolnum $data_poolnum --yes-i-really-mean-it 2>$TMPFILE

  # Check that 'fs reset' runs
  stone fs reset $FS_NAME --yes-i-really-mean-it

  # Check that creating a second FS fails by default
  stone osd pool create fs_metadata2 16
  stone osd pool create fs_data2 16
  set +e
  expect_false stone fs new stonefs2 fs_metadata2 fs_data2
  set -e

  # Check that setting enable_multiple enables creation of second fs
  stone fs flag set enable_multiple true --yes-i-really-mean-it
  stone fs new stonefs2 fs_metadata2 fs_data2

  # Clean up multi-fs stuff
  fail_all_mds stonefs2
  stone fs rm stonefs2 --yes-i-really-mean-it
  stone osd pool delete fs_metadata2 fs_metadata2 --yes-i-really-really-mean-it
  stone osd pool delete fs_data2 fs_data2 --yes-i-really-really-mean-it

  fail_all_mds $FS_NAME

  # Clean up to enable subsequent fs new tests
  stone fs rm $FS_NAME --yes-i-really-mean-it

  set +e
  stone fs new $FS_NAME fs_metadata mds-ec-pool --force 2>$TMPFILE
  check_response 'erasure-code' $? 22
  stone fs new $FS_NAME mds-ec-pool fs_data 2>$TMPFILE
  check_response 'erasure-code' $? 22
  stone fs new $FS_NAME mds-ec-pool mds-ec-pool 2>$TMPFILE
  check_response 'erasure-code' $? 22
  set -e

  # ... new create a cache tier in front of the EC pool...
  stone osd pool create mds-tier 2
  stone osd tier add mds-ec-pool mds-tier
  stone osd tier set-overlay mds-ec-pool mds-tier
  tier_poolnum=$(stone osd dump | grep "pool.* 'mds-tier" | awk '{print $2;}')

  # Use of a readonly tier should be forbidden
  stone osd tier cache-mode mds-tier readonly --yes-i-really-mean-it
  set +e
  stone fs new $FS_NAME fs_metadata mds-ec-pool --force 2>$TMPFILE
  check_response 'has a write tier (mds-tier) that is configured to forward' $? 22
  set -e

  # Use of a writeback tier should enable FS creation
  stone osd tier cache-mode mds-tier writeback
  stone fs new $FS_NAME fs_metadata mds-ec-pool --force

  # While a FS exists using the tiered pools, I should not be allowed
  # to remove the tier
  set +e
  stone osd tier remove-overlay mds-ec-pool 2>$TMPFILE
  check_response 'in use by StoneFS' $? 16
  stone osd tier remove mds-ec-pool mds-tier 2>$TMPFILE
  check_response 'in use by StoneFS' $? 16
  set -e

  fail_all_mds $FS_NAME
  stone fs rm $FS_NAME --yes-i-really-mean-it

  # ... but we should be forbidden from using the cache pool in the FS directly.
  set +e
  stone fs new $FS_NAME fs_metadata mds-tier --force 2>$TMPFILE
  check_response 'in use as a cache tier' $? 22
  stone fs new $FS_NAME mds-tier fs_data 2>$TMPFILE
  check_response 'in use as a cache tier' $? 22
  stone fs new $FS_NAME mds-tier mds-tier 2>$TMPFILE
  check_response 'in use as a cache tier' $? 22
  set -e

  # Clean up tier + EC pools
  stone osd tier remove-overlay mds-ec-pool
  stone osd tier remove mds-ec-pool mds-tier

  # Create a FS using the 'cache' pool now that it's no longer a tier
  stone fs new $FS_NAME fs_metadata mds-tier --force

  # We should be forbidden from using this pool as a tier now that
  # it's in use for StoneFS
  set +e
  stone osd tier add mds-ec-pool mds-tier 2>$TMPFILE
  check_response 'in use by StoneFS' $? 16
  set -e

  fail_all_mds $FS_NAME
  stone fs rm $FS_NAME --yes-i-really-mean-it

  # We should be permitted to use an EC pool with overwrites enabled
  # as the data pool...
  stone osd pool set mds-ec-pool allow_ec_overwrites true
  stone fs new $FS_NAME fs_metadata mds-ec-pool --force 2>$TMPFILE
  fail_all_mds $FS_NAME
  stone fs rm $FS_NAME --yes-i-really-mean-it

  # ...but not as the metadata pool
  set +e
  stone fs new $FS_NAME mds-ec-pool fs_data 2>$TMPFILE
  check_response 'erasure-code' $? 22
  set -e

  stone osd pool delete mds-ec-pool mds-ec-pool --yes-i-really-really-mean-it

  # Create a FS and check that we can subsequently add a cache tier to it
  stone fs new $FS_NAME fs_metadata fs_data --force

  # Adding overlay to FS pool should be permitted, RADOS clients handle this.
  stone osd tier add fs_metadata mds-tier
  stone osd tier cache-mode mds-tier writeback
  stone osd tier set-overlay fs_metadata mds-tier

  # Removing tier should be permitted because the underlying pool is
  # replicated (#11504 case)
  stone osd tier cache-mode mds-tier readproxy
  stone osd tier remove-overlay fs_metadata
  stone osd tier remove fs_metadata mds-tier
  stone osd pool delete mds-tier mds-tier --yes-i-really-really-mean-it

  # Clean up FS
  fail_all_mds $FS_NAME
  stone fs rm $FS_NAME --yes-i-really-mean-it



  stone mds stat
  # stone mds tell mds.a getmap
  # stone mds rm
  # stone mds rmfailed
  # stone mds set_state

  stone osd pool delete fs_data fs_data --yes-i-really-really-mean-it
  stone osd pool delete fs_metadata fs_metadata --yes-i-really-really-mean-it
}

function test_mon_mds_metadata()
{
  local nmons=$(stone tell 'mon.*' version | grep -c 'version')
  test "$nmons" -gt 0

  stone fs dump |
  sed -nEe "s/^([0-9]+):.*'([a-z])' mds\\.([0-9]+)\\..*/\\1 \\2 \\3/p" |
  while read gid id rank; do
    stone mds metadata ${gid} | grep '"hostname":'
    stone mds metadata ${id} | grep '"hostname":'
    stone mds metadata ${rank} | grep '"hostname":'

    local n=$(stone tell 'mon.*' mds metadata ${id} | grep -c '"hostname":')
    test "$n" -eq "$nmons"
  done

  expect_false stone mds metadata UNKNOWN
}

function test_mon_mon()
{
  # print help message
  stone --help mon
  # no mon add/remove
  stone mon dump
  stone mon getmap -o $TEMP_DIR/monmap.$$
  [ -s $TEMP_DIR/monmap.$$ ]

  # stone mon tell
  first=$(stone mon dump -f json | jq -r '.mons[0].name')
  stone tell mon.$first mon_status

  # test mon features
  stone mon feature ls
  stone mon feature set kraken --yes-i-really-mean-it
  expect_false stone mon feature set abcd
  expect_false stone mon feature set abcd --yes-i-really-mean-it

  # test elector
  expect_failure $TEMP_DIR stone mon add disallowed_leader $first
  stone mon set election_strategy disallow
  stone mon add disallowed_leader $first
  stone mon set election_strategy connectivity
  stone mon rm disallowed_leader $first
  stone mon set election_strategy classic
  expect_failure $TEMP_DIR stone mon rm disallowed_leader $first

  # test mon stat
  # don't check output, just ensure it does not fail.
  stone mon stat
  stone mon stat -f json | jq '.'
}

function test_mon_priority_and_weight()
{
    for i in 0 1 65535; do
      stone mon set-weight a $i
      w=$(stone mon dump --format=json-pretty 2>/dev/null | jq '.mons[0].weight')
      [[ "$w" == "$i" ]]
    done

    for i in -1 65536; do
      expect_false stone mon set-weight a $i
    done
}

function gen_secrets_file()
{
  # lets assume we can have the following types
  #  all - generates both stonex and lockbox, with mock dm-crypt key
  #  stonex - only stonex
  #  no_stonex - lockbox and dm-crypt, no stonex
  #  no_lockbox - dm-crypt and stonex, no lockbox
  #  empty - empty file
  #  empty_json - correct json, empty map
  #  bad_json - bad json :)
  #
  local t=$1
  if [[ -z "$t" ]]; then
    t="all"
  fi

  fn=$(mktemp $TEMP_DIR/secret.XXXXXX)
  echo $fn
  if [[ "$t" == "empty" ]]; then
    return 0
  fi

  echo "{" > $fn
  if [[ "$t" == "bad_json" ]]; then
    echo "asd: ; }" >> $fn
    return 0
  elif [[ "$t" == "empty_json" ]]; then
    echo "}" >> $fn
    return 0
  fi

  stonex_secret="\"stonex_secret\": \"$(stone-authtool --gen-print-key)\""
  lb_secret="\"stonex_lockbox_secret\": \"$(stone-authtool --gen-print-key)\""
  dmcrypt_key="\"dmcrypt_key\": \"$(stone-authtool --gen-print-key)\""

  if [[ "$t" == "all" ]]; then
    echo "$stonex_secret,$lb_secret,$dmcrypt_key" >> $fn
  elif [[ "$t" == "stonex" ]]; then
    echo "$stonex_secret" >> $fn
  elif [[ "$t" == "no_stonex" ]]; then
    echo "$lb_secret,$dmcrypt_key" >> $fn
  elif [[ "$t" == "no_lockbox" ]]; then
    echo "$stonex_secret,$dmcrypt_key" >> $fn
  else
    echo "unknown gen_secrets_file() type \'$fn\'"
    return 1
  fi
  echo "}" >> $fn
  return 0
}

function test_mon_osd_create_destroy()
{
  stone osd new 2>&1 | grep 'EINVAL'
  stone osd new '' -1 2>&1 | grep 'EINVAL'
  stone osd new '' 10 2>&1 | grep 'EINVAL'

  old_maxosd=$(stone osd getmaxosd | sed -e 's/max_osd = //' -e 's/ in epoch.*//')

  old_osds=$(stone osd ls)
  num_osds=$(stone osd ls | wc -l)

  uuid=$(uuidgen)
  id=$(stone osd new $uuid 2>/dev/null)

  for i in $old_osds; do
    [[ "$i" != "$id" ]]
  done

  stone osd find $id

  id2=`stone osd new $uuid 2>/dev/null`

  [[ $id2 == $id ]]

  stone osd new $uuid $id

  id3=$(stone osd getmaxosd | sed -e 's/max_osd = //' -e 's/ in epoch.*//')
  stone osd new $uuid $((id3+1)) 2>&1 | grep EEXIST

  uuid2=$(uuidgen)
  id2=$(stone osd new $uuid2)
  stone osd find $id2
  [[ "$id2" != "$id" ]]

  stone osd new $uuid $id2 2>&1 | grep EEXIST
  stone osd new $uuid2 $id2

  # test with secrets
  empty_secrets=$(gen_secrets_file "empty")
  empty_json=$(gen_secrets_file "empty_json")
  all_secrets=$(gen_secrets_file "all")
  stonex_only=$(gen_secrets_file "stonex")
  no_stonex=$(gen_secrets_file "no_stonex")
  no_lockbox=$(gen_secrets_file "no_lockbox")
  bad_json=$(gen_secrets_file "bad_json")

  # empty secrets should be idempotent
  new_id=$(stone osd new $uuid $id -i $empty_secrets)
  [[ "$new_id" == "$id" ]]

  # empty json, thus empty secrets
  new_id=$(stone osd new $uuid $id -i $empty_json)
  [[ "$new_id" == "$id" ]]

  stone osd new $uuid $id -i $all_secrets 2>&1 | grep 'EEXIST'

  stone osd rm $id
  stone osd rm $id2
  stone osd setmaxosd $old_maxosd

  stone osd new $uuid -i $no_stonex 2>&1 | grep 'EINVAL'
  stone osd new $uuid -i $no_lockbox 2>&1 | grep 'EINVAL'

  osds=$(stone osd ls)
  id=$(stone osd new $uuid -i $all_secrets)
  for i in $osds; do
    [[ "$i" != "$id" ]]
  done

  stone osd find $id

  # validate secrets and dm-crypt are set
  k=$(stone auth get-key osd.$id --format=json-pretty 2>/dev/null | jq '.key')
  s=$(cat $all_secrets | jq '.stonex_secret')
  [[ $k == $s ]]
  k=$(stone auth get-key client.osd-lockbox.$uuid --format=json-pretty 2>/dev/null | \
      jq '.key')
  s=$(cat $all_secrets | jq '.stonex_lockbox_secret')
  [[ $k == $s ]]
  stone config-key exists dm-crypt/osd/$uuid/luks

  osds=$(stone osd ls)
  id2=$(stone osd new $uuid2 -i $stonex_only)
  for i in $osds; do
    [[ "$i" != "$id2" ]]
  done

  stone osd find $id2
  k=$(stone auth get-key osd.$id --format=json-pretty 2>/dev/null | jq '.key')
  s=$(cat $all_secrets | jq '.stonex_secret')
  [[ $k == $s ]]
  expect_false stone auth get-key client.osd-lockbox.$uuid2
  expect_false stone config-key exists dm-crypt/osd/$uuid2/luks

  stone osd destroy osd.$id2 --yes-i-really-mean-it
  stone osd destroy $id2 --yes-i-really-mean-it
  stone osd find $id2
  expect_false stone auth get-key osd.$id2
  stone osd dump | grep osd.$id2 | grep destroyed

  id3=$id2
  uuid3=$(uuidgen)
  stone osd new $uuid3 $id3 -i $all_secrets
  stone osd dump | grep osd.$id3 | expect_false grep destroyed
  stone auth get-key client.osd-lockbox.$uuid3
  stone auth get-key osd.$id3
  stone config-key exists dm-crypt/osd/$uuid3/luks

  stone osd purge-new osd.$id3 --yes-i-really-mean-it
  expect_false stone osd find $id2
  expect_false stone auth get-key osd.$id2
  expect_false stone auth get-key client.osd-lockbox.$uuid3
  expect_false stone config-key exists dm-crypt/osd/$uuid3/luks
  stone osd purge osd.$id3 --yes-i-really-mean-it
  stone osd purge-new osd.$id3 --yes-i-really-mean-it # idempotent

  stone osd purge osd.$id --yes-i-really-mean-it
  stone osd purge 123456 --yes-i-really-mean-it
  expect_false stone osd find $id
  expect_false stone auth get-key osd.$id
  expect_false stone auth get-key client.osd-lockbox.$uuid
  expect_false stone config-key exists dm-crypt/osd/$uuid/luks

  rm $empty_secrets $empty_json $all_secrets $stonex_only \
     $no_stonex $no_lockbox $bad_json

  for i in $(stone osd ls); do
    [[ "$i" != "$id" ]]
    [[ "$i" != "$id2" ]]
    [[ "$i" != "$id3" ]]
  done

  [[ "$(stone osd ls | wc -l)" == "$num_osds" ]]
  stone osd setmaxosd $old_maxosd

}

function test_mon_config_key()
{
  key=asdfasdfqwerqwreasdfuniquesa123df
  stone config-key list | grep -c $key | grep 0
  stone config-key get $key | grep -c bar | grep 0
  stone config-key set $key bar
  stone config-key get $key | grep bar
  stone config-key list | grep -c $key | grep 1
  stone config-key dump | grep $key | grep bar
  stone config-key rm $key
  expect_false stone config-key get $key
  stone config-key list | grep -c $key | grep 0
  stone config-key dump | grep -c $key | grep 0
}

function test_mon_osd()
{
  #
  # osd blocklist
  #
  bl=192.168.0.1:0/1000
  stone osd blocklist add $bl
  stone osd blocklist ls | grep $bl
  stone osd blocklist ls --format=json-pretty  | sed 's/\\\//\//' | grep $bl
  stone osd dump --format=json-pretty | grep $bl
  stone osd dump | grep $bl
  stone osd blocklist rm $bl
  stone osd blocklist ls | expect_false grep $bl

  bl=192.168.0.1
  # test without nonce, invalid nonce
  stone osd blocklist add $bl
  stone osd blocklist ls | grep $bl
  stone osd blocklist rm $bl
  stone osd blocklist ls | expect_false grep $bl
  expect_false "stone osd blocklist $bl/-1"
  expect_false "stone osd blocklist $bl/foo"

  # test with wrong address
  expect_false "stone osd blocklist 1234.56.78.90/100"

  # Test `clear`
  stone osd blocklist add $bl
  stone osd blocklist ls | grep $bl
  stone osd blocklist clear
  stone osd blocklist ls | expect_false grep $bl

  # deprecated syntax?
  stone osd blacklist ls

  #
  # osd crush
  #
  stone osd crush reweight-all
  stone osd crush tunables legacy
  stone osd crush show-tunables | grep argonaut
  stone osd crush tunables bobtail
  stone osd crush show-tunables | grep bobtail
  stone osd crush tunables firefly
  stone osd crush show-tunables | grep firefly

  stone osd crush set-tunable straw_calc_version 0
  stone osd crush get-tunable straw_calc_version | grep 0
  stone osd crush set-tunable straw_calc_version 1
  stone osd crush get-tunable straw_calc_version | grep 1

  #
  # require-min-compat-client
  expect_false stone osd set-require-min-compat-client dumpling  # firefly tunables
  stone osd get-require-min-compat-client | grep luminous
  stone osd dump | grep 'require_min_compat_client luminous'

  #
  # osd scrub
  #

  # blocking
  stone osd scrub 0 --block
  stone osd deep-scrub 0 --block

  # how do I tell when these are done?
  stone osd scrub 0
  stone osd deep-scrub 0
  stone osd repair 0

  # pool scrub, force-recovery/backfill
  pool_names=`rados lspools`
  for pool_name in $pool_names
  do
    stone osd pool scrub $pool_name
    stone osd pool deep-scrub $pool_name
    stone osd pool repair $pool_name
    stone osd pool force-recovery $pool_name
    stone osd pool cancel-force-recovery $pool_name
    stone osd pool force-backfill $pool_name
    stone osd pool cancel-force-backfill $pool_name
  done

  for f in noup nodown noin noout noscrub nodeep-scrub nobackfill \
	  norebalance norecover notieragent
  do
    stone osd set $f
    stone osd unset $f
  done
  expect_false stone osd set bogus
  expect_false stone osd unset bogus
  for f in sortbitwise recover_deletes require_jewel_osds \
	  require_kraken_osds
  do
	expect_false stone osd set $f
	expect_false stone osd unset $f
  done
  stone osd require-osd-release pacific
  # can't lower
  expect_false stone osd require-osd-release octopus
  expect_false stone osd require-osd-release nautilus
  expect_false stone osd require-osd-release mimic
  expect_false stone osd require-osd-release luminous
  # these are no-ops but should succeed.

  stone osd set noup
  stone osd down 0
  stone osd dump | grep 'osd.0 down'
  stone osd unset noup
  max_run=1000
  for ((i=0; i < $max_run; i++)); do
    if ! stone osd dump | grep 'osd.0 up'; then
      echo "waiting for osd.0 to come back up ($i/$max_run)"
      sleep 1
    else
      break
    fi
  done
  stone osd dump | grep 'osd.0 up'

  stone osd dump | grep 'osd.0 up'
  # stone osd find expects the OsdName, so both ints and osd.n should work.
  stone osd find 1
  stone osd find osd.1
  expect_false stone osd find osd.xyz
  expect_false stone osd find xyz
  expect_false stone osd find 0.1
  stone --format plain osd find 1 # falls back to json-pretty
  if [ `uname` == Linux ]; then
    stone osd metadata 1 | grep 'distro'
    stone --format plain osd metadata 1 | grep 'distro' # falls back to json-pretty
  fi
  stone osd out 0
  stone osd dump | grep 'osd.0.*out'
  stone osd in 0
  stone osd dump | grep 'osd.0.*in'
  stone osd find 0

  stone osd info 0
  stone osd info osd.0
  expect_false stone osd info osd.xyz
  expect_false stone osd info xyz
  expect_false stone osd info 42
  expect_false stone osd info osd.42

  stone osd info
  info_json=$(stone osd info --format=json | jq -cM '.')
  dump_json=$(stone osd dump --format=json | jq -cM '.osds')
  [[ "${info_json}" == "${dump_json}" ]]

  info_json=$(stone osd info 0 --format=json | jq -cM '.')
  dump_json=$(stone osd dump --format=json | \
	  jq -cM '.osds[] | select(.osd == 0)')
  [[ "${info_json}" == "${dump_json}" ]]
  
  info_plain="$(stone osd info)"
  dump_plain="$(stone osd dump | grep '^osd')"
  [[ "${info_plain}" == "${dump_plain}" ]]

  info_plain="$(stone osd info 0)"
  dump_plain="$(stone osd dump | grep '^osd.0')"
  [[ "${info_plain}" == "${dump_plain}" ]]

  stone osd add-nodown 0 1
  stone health detail | grep 'NODOWN'
  stone osd rm-nodown 0 1
  ! stone health detail | grep 'NODOWN'

  stone osd out 0 # so we can mark it as noin later
  stone osd add-noin 0
  stone health detail | grep 'NOIN'
  stone osd rm-noin 0
  ! stone health detail | grep 'NOIN'
  stone osd in 0

  stone osd add-noout 0
  stone health detail | grep 'NOOUT'
  stone osd rm-noout 0
  ! stone health detail | grep 'NOOUT'

  # test osd id parse
  expect_false stone osd add-noup 797er
  expect_false stone osd add-nodown u9uwer
  expect_false stone osd add-noin 78~15

  expect_false stone osd rm-noup 1234567
  expect_false stone osd rm-nodown fsadf7
  expect_false stone osd rm-noout 790-fd

  ids=`stone osd ls-tree default`
  for osd in $ids
  do
    stone osd add-nodown $osd
    stone osd add-noout $osd
  done
  stone -s | grep 'NODOWN'
  stone -s | grep 'NOOUT'
  stone osd rm-nodown any
  stone osd rm-noout all
  ! stone -s | grep 'NODOWN'
  ! stone -s | grep 'NOOUT'

  # test crush node flags
  stone osd add-noup osd.0
  stone osd add-nodown osd.0
  stone osd add-noin osd.0
  stone osd add-noout osd.0
  stone osd dump -f json-pretty | jq ".crush_node_flags" | expect_false grep "osd.0"
  stone osd rm-noup osd.0
  stone osd rm-nodown osd.0
  stone osd rm-noin osd.0
  stone osd rm-noout osd.0
  stone osd dump -f json-pretty | jq ".crush_node_flags" | expect_false grep "osd.0"

  stone osd crush add-bucket foo host root=default
  stone osd add-noup foo
  stone osd add-nodown foo
  stone osd add-noin foo
  stone osd add-noout foo
  stone osd dump -f json-pretty | jq ".crush_node_flags" | grep foo
  stone osd rm-noup foo
  stone osd rm-nodown foo
  stone osd rm-noin foo
  stone osd rm-noout foo
  stone osd dump -f json-pretty | jq ".crush_node_flags" | expect_false grep foo
  stone osd add-noup foo
  stone osd dump -f json-pretty | jq ".crush_node_flags" | grep foo
  stone osd crush rm foo
  stone osd dump -f json-pretty | jq ".crush_node_flags" | expect_false grep foo

  stone osd set-group noup osd.0
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noup'
  stone osd set-group noup,nodown osd.0
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noup'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'nodown'
  stone osd set-group noup,nodown,noin osd.0
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noup'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noin'
  stone osd set-group noup,nodown,noin,noout osd.0
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noup'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noin'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noout'
  stone osd unset-group noup osd.0
  stone osd dump -f json-pretty | jq ".osds[0].state" | expect_false grep 'noup'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noin'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noout'
  stone osd unset-group noup,nodown osd.0
  stone osd dump -f json-pretty | jq ".osds[0].state" | expect_false grep 'noup\|nodown'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noin'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noout'
  stone osd unset-group noup,nodown,noin osd.0
  stone osd dump -f json-pretty | jq ".osds[0].state" | expect_false grep 'noup\|nodown\|noin'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noout'
  stone osd unset-group noup,nodown,noin,noout osd.0
  stone osd dump -f json-pretty | jq ".osds[0].state" | expect_false grep 'noup\|nodown\|noin\|noout'

  stone osd set-group noup,nodown,noin,noout osd.0 osd.1
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noup'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noin'
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noout'
  stone osd dump -f json-pretty | jq ".osds[1].state" | grep 'noup'
  stone osd dump -f json-pretty | jq ".osds[1].state" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".osds[1].state" | grep 'noin'
  stone osd dump -f json-pretty | jq ".osds[1].state" | grep 'noout'
  stone osd unset-group noup,nodown,noin,noout osd.0 osd.1
  stone osd dump -f json-pretty | jq ".osds[0].state" | expect_false grep 'noup\|nodown\|noin\|noout'
  stone osd dump -f json-pretty | jq ".osds[1].state" | expect_false grep 'noup\|nodown\|noin\|noout'

  stone osd set-group noup all
  stone osd dump -f json-pretty | jq ".osds[0].state" | grep 'noup'
  stone osd unset-group noup all
  stone osd dump -f json-pretty | jq ".osds[0].state" | expect_false grep 'noup'

  # crush node flags
  stone osd crush add-bucket foo host root=default
  stone osd set-group noup foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noup'
  stone osd set-group noup,nodown foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noup'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'nodown'
  stone osd set-group noup,nodown,noin foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noup'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noin'
  stone osd set-group noup,nodown,noin,noout foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noup'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noin'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noout'

  stone osd unset-group noup foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | expect_false grep 'noup'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noin'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noout'
  stone osd unset-group noup,nodown foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | expect_false grep 'noup\|nodown'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noin'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noout'
  stone osd unset-group noup,nodown,noin foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | expect_false grep 'noup\|nodown\|noin'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noout'
  stone osd unset-group noup,nodown,noin,noout foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | expect_false grep 'noup\|nodown\|noin\|noout'

  stone osd set-group noin,noout foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noin'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noout'
  stone osd unset-group noin,noout foo
  stone osd dump -f json-pretty | jq ".crush_node_flags" | expect_false grep 'foo'

  stone osd set-group noup,nodown,noin,noout foo
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noup'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noin'
  stone osd dump -f json-pretty | jq ".crush_node_flags.foo" | grep 'noout'
  stone osd crush rm foo
  stone osd dump -f json-pretty | jq ".crush_node_flags" | expect_false grep 'foo'

  # test device class flags
  osd_0_device_class=$(stone osd crush get-device-class osd.0)
  stone osd set-group noup $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noup'
  stone osd set-group noup,nodown $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noup'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'nodown'
  stone osd set-group noup,nodown,noin $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noup'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noin'
  stone osd set-group noup,nodown,noin,noout $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noup'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noin'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noout'

  stone osd unset-group noup $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | expect_false grep 'noup'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'nodown'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noin'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noout'
  stone osd unset-group noup,nodown $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | expect_false grep 'noup\|nodown'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noin'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noout'
  stone osd unset-group noup,nodown,noin $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | expect_false grep 'noup\|nodown\|noin'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noout'
  stone osd unset-group noup,nodown,noin,noout $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | expect_false grep 'noup\|nodown\|noin\|noout'

  stone osd set-group noin,noout $osd_0_device_class
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noin'
  stone osd dump -f json-pretty | jq ".device_class_flags.$osd_0_device_class" | grep 'noout'
  stone osd unset-group noin,noout $osd_0_device_class
  stone osd dump -f json-pretty | jq ".crush_node_flags" | expect_false grep $osd_0_device_class

  # make sure mark out preserves weight
  stone osd reweight osd.0 .5
  stone osd dump | grep ^osd.0 | grep 'weight 0.5'
  stone osd out 0
  stone osd in 0
  stone osd dump | grep ^osd.0 | grep 'weight 0.5'

  stone osd getmap -o $f
  [ -s $f ]
  rm $f
  save=$(stone osd getmaxosd | sed -e 's/max_osd = //' -e 's/ in epoch.*//')
  [ "$save" -gt 0 ]
  stone osd setmaxosd $((save - 1)) 2>&1 | grep 'EBUSY'
  stone osd setmaxosd 10
  stone osd getmaxosd | grep 'max_osd = 10'
  stone osd setmaxosd $save
  stone osd getmaxosd | grep "max_osd = $save"

  for id in `stone osd ls` ; do
    retry_eagain 5 map_enxio_to_eagain stone tell osd.$id version
  done

  stone osd rm 0 2>&1 | grep 'EBUSY'

  local old_osds=$(echo $(stone osd ls))
  id=`stone osd create`
  stone osd find $id
  stone osd lost $id --yes-i-really-mean-it
  expect_false stone osd setmaxosd $id
  local new_osds=$(echo $(stone osd ls))
  for id in $(echo $new_osds | sed -e "s/$old_osds//") ; do
      stone osd rm $id
  done

  uuid=`uuidgen`
  id=`stone osd create $uuid`
  id2=`stone osd create $uuid`
  [ "$id" = "$id2" ]
  stone osd rm $id

  stone --help osd

  # reset max_osd.
  stone osd setmaxosd $id
  stone osd getmaxosd | grep "max_osd = $save"
  local max_osd=$save

  stone osd create $uuid 0 2>&1 | grep 'EINVAL'
  stone osd create $uuid $((max_osd - 1)) 2>&1 | grep 'EINVAL'

  id=`stone osd create $uuid $max_osd`
  [ "$id" = "$max_osd" ]
  stone osd find $id
  max_osd=$((max_osd + 1))
  stone osd getmaxosd | grep "max_osd = $max_osd"

  stone osd create $uuid $((id - 1)) 2>&1 | grep 'EEXIST'
  stone osd create $uuid $((id + 1)) 2>&1 | grep 'EEXIST'
  id2=`stone osd create $uuid`
  [ "$id" = "$id2" ]
  id2=`stone osd create $uuid $id`
  [ "$id" = "$id2" ]

  uuid=`uuidgen`
  local gap_start=$max_osd
  id=`stone osd create $uuid $((gap_start + 100))`
  [ "$id" = "$((gap_start + 100))" ]
  max_osd=$((id + 1))
  stone osd getmaxosd | grep "max_osd = $max_osd"

  stone osd create $uuid $gap_start 2>&1 | grep 'EEXIST'

  #
  # When STONE_CLI_TEST_DUP_COMMAND is set, osd create
  # is repeated and consumes two osd id, not just one.
  #
  local next_osd=$gap_start
  id=`stone osd create $(uuidgen)`
  [ "$id" = "$next_osd" ]

  next_osd=$((id + 1))
  id=`stone osd create $(uuidgen) $next_osd`
  [ "$id" = "$next_osd" ]

  local new_osds=$(echo $(stone osd ls))
  for id in $(echo $new_osds | sed -e "s/$old_osds//") ; do
      [ $id -ge $save ]
      stone osd rm $id
  done
  stone osd setmaxosd $save

  stone osd ls
  stone osd pool create data 16
  stone osd pool application enable data rados
  stone osd lspools | grep data
  stone osd map data foo | grep 'pool.*data.*object.*foo.*pg.*up.*acting'
  stone osd map data foo namespace| grep 'pool.*data.*object.*namespace/foo.*pg.*up.*acting'
  stone osd pool delete data data --yes-i-really-really-mean-it

  stone osd pause
  stone osd dump | grep 'flags.*pauserd,pausewr'
  stone osd unpause

  stone osd tree
  stone osd tree up
  stone osd tree down
  stone osd tree in
  stone osd tree out
  stone osd tree destroyed
  stone osd tree up in
  stone osd tree up out
  stone osd tree down in
  stone osd tree down out
  stone osd tree out down
  expect_false stone osd tree up down
  expect_false stone osd tree up destroyed
  expect_false stone osd tree down destroyed
  expect_false stone osd tree up down destroyed
  expect_false stone osd tree in out
  expect_false stone osd tree up foo

  stone osd metadata
  stone osd count-metadata os
  stone osd versions

  stone osd perf
  stone osd blocked-by

  stone osd stat | grep up
}

function test_mon_crush()
{
  f=$TEMP_DIR/map.$$
  epoch=$(stone osd getcrushmap -o $f 2>&1 | tail -n1)
  [ -s $f ]
  [ "$epoch" -gt 1 ]
  nextepoch=$(( $epoch + 1 ))
  echo epoch $epoch nextepoch $nextepoch
  rm -f $f.epoch
  expect_false stone osd setcrushmap $nextepoch -i $f
  gotepoch=$(stone osd setcrushmap $epoch -i $f 2>&1 | tail -n1)
  echo gotepoch $gotepoch
  [ "$gotepoch" -eq "$nextepoch" ]
  # should be idempotent
  gotepoch=$(stone osd setcrushmap $epoch -i $f 2>&1 | tail -n1)
  echo epoch $gotepoch
  [ "$gotepoch" -eq "$nextepoch" ]
  rm $f
}

function test_mon_osd_pool()
{
  #
  # osd pool
  #
  stone osd pool create data 16
  stone osd pool application enable data rados
  stone osd pool mksnap data datasnap
  rados -p data lssnap | grep datasnap
  stone osd pool rmsnap data datasnap
  expect_false stone osd pool rmsnap pool_fake snapshot
  stone osd pool delete data data --yes-i-really-really-mean-it

  stone osd pool create data2 16
  stone osd pool application enable data2 rados
  stone osd pool rename data2 data3
  stone osd lspools | grep data3
  stone osd pool delete data3 data3 --yes-i-really-really-mean-it

  stone osd pool create replicated 16 16 replicated
  stone osd pool create replicated 1 16 replicated
  stone osd pool create replicated 16 16 # default is replicated
  stone osd pool create replicated 16    # default is replicated, pgp_num = pg_num
  stone osd pool application enable replicated rados
  # should fail because the type is not the same
  expect_false stone osd pool create replicated 16 16 erasure
  stone osd lspools | grep replicated
  stone osd pool create ec_test 1 1 erasure
  stone osd pool application enable ec_test rados
  set +e
  stone osd count-metadata osd_objectstore | grep 'bluestore'
  if [ $? -eq 1 ]; then # enable ec_overwrites on non-bluestore pools should fail
      stone osd pool set ec_test allow_ec_overwrites true >& $TMPFILE
      check_response "pool must only be stored on bluestore for scrubbing to work" $? 22
  else
      stone osd pool set ec_test allow_ec_overwrites true || return 1
      expect_false stone osd pool set ec_test allow_ec_overwrites false
  fi
  set -e
  stone osd pool delete replicated replicated --yes-i-really-really-mean-it
  stone osd pool delete ec_test ec_test --yes-i-really-really-mean-it

  # test create pool with rule
  stone osd erasure-code-profile set foo foo
  stone osd erasure-code-profile ls | grep foo
  stone osd crush rule create-erasure foo foo
  stone osd pool create erasure 16 16 erasure foo
  expect_false stone osd erasure-code-profile rm foo
  stone osd pool delete erasure erasure --yes-i-really-really-mean-it
  stone osd crush rule rm foo
  stone osd erasure-code-profile rm foo

  # autoscale mode
  stone osd pool create modeon --autoscale-mode=on
  stone osd dump | grep modeon | grep 'autoscale_mode on'
  stone osd pool create modewarn --autoscale-mode=warn
  stone osd dump | grep modewarn | grep 'autoscale_mode warn'
  stone osd pool create modeoff --autoscale-mode=off
  stone osd dump | grep modeoff | grep 'autoscale_mode off'
  stone osd pool delete modeon modeon --yes-i-really-really-mean-it
  stone osd pool delete modewarn modewarn --yes-i-really-really-mean-it
  stone osd pool delete modeoff modeoff --yes-i-really-really-mean-it
}

function test_mon_osd_pool_quota()
{
  #
  # test osd pool set/get quota
  #

  # create tmp pool
  stone osd pool create tmp-quota-pool 32
  stone osd pool application enable tmp-quota-pool rados
  #
  # set erroneous quotas
  #
  expect_false stone osd pool set-quota tmp-quota-pool max_fooness 10
  expect_false stone osd pool set-quota tmp-quota-pool max_bytes -1
  expect_false stone osd pool set-quota tmp-quota-pool max_objects aaa
  #
  # set valid quotas
  #
  stone osd pool set-quota tmp-quota-pool max_bytes 10
  stone osd pool set-quota tmp-quota-pool max_objects 10M
  #
  # get quotas in json-pretty format
  #
  stone osd pool get-quota tmp-quota-pool --format=json-pretty | \
    grep '"quota_max_objects":.*10000000'
  stone osd pool get-quota tmp-quota-pool --format=json-pretty | \
    grep '"quota_max_bytes":.*10'
  #
  # get quotas
  #
  stone osd pool get-quota tmp-quota-pool | grep 'max bytes.*10 B'
  stone osd pool get-quota tmp-quota-pool | grep 'max objects.*10.*M objects'
  #
  # set valid quotas with unit prefix
  #
  stone osd pool set-quota tmp-quota-pool max_bytes 10K
  #
  # get quotas
  #
  stone osd pool get-quota tmp-quota-pool | grep 'max bytes.*10 Ki'
  #
  # set valid quotas with unit prefix
  #
  stone osd pool set-quota tmp-quota-pool max_bytes 10Ki
  #
  # get quotas
  #
  stone osd pool get-quota tmp-quota-pool | grep 'max bytes.*10 Ki'
  #
  #
  # reset pool quotas
  #
  stone osd pool set-quota tmp-quota-pool max_bytes 0
  stone osd pool set-quota tmp-quota-pool max_objects 0
  #
  # test N/A quotas
  #
  stone osd pool get-quota tmp-quota-pool | grep 'max bytes.*N/A'
  stone osd pool get-quota tmp-quota-pool | grep 'max objects.*N/A'
  #
  # cleanup tmp pool
  stone osd pool delete tmp-quota-pool tmp-quota-pool --yes-i-really-really-mean-it
}

function test_mon_pg()
{
  # Make sure we start healthy.
  wait_for_health_ok

  stone pg debug unfound_objects_exist
  stone pg debug degraded_pgs_exist
  stone pg deep-scrub 1.0
  stone pg dump
  stone pg dump pgs_brief --format=json
  stone pg dump pgs --format=json
  stone pg dump pools --format=json
  stone pg dump osds --format=json
  stone pg dump sum --format=json
  stone pg dump all --format=json
  stone pg dump pgs_brief osds --format=json
  stone pg dump pools osds pgs_brief --format=json
  stone pg dump_json
  stone pg dump_pools_json
  stone pg dump_stuck inactive
  stone pg dump_stuck unclean
  stone pg dump_stuck stale
  stone pg dump_stuck undersized
  stone pg dump_stuck degraded
  stone pg ls
  stone pg ls 1
  stone pg ls stale
  expect_false stone pg ls scrubq
  stone pg ls active stale repair recovering
  stone pg ls 1 active
  stone pg ls 1 active stale
  stone pg ls-by-primary osd.0
  stone pg ls-by-primary osd.0 1
  stone pg ls-by-primary osd.0 active
  stone pg ls-by-primary osd.0 active stale
  stone pg ls-by-primary osd.0 1 active stale
  stone pg ls-by-osd osd.0
  stone pg ls-by-osd osd.0 1
  stone pg ls-by-osd osd.0 active
  stone pg ls-by-osd osd.0 active stale
  stone pg ls-by-osd osd.0 1 active stale
  stone pg ls-by-pool rbd
  stone pg ls-by-pool rbd active stale
  # can't test this...
  # stone pg force_create_pg
  stone pg getmap -o $TEMP_DIR/map.$$
  [ -s $TEMP_DIR/map.$$ ]
  stone pg map 1.0 | grep acting
  stone pg repair 1.0
  stone pg scrub 1.0

  stone osd set-full-ratio .962
  stone osd dump | grep '^full_ratio 0.962'
  stone osd set-backfillfull-ratio .912
  stone osd dump | grep '^backfillfull_ratio 0.912'
  stone osd set-nearfull-ratio .892
  stone osd dump | grep '^nearfull_ratio 0.892'

  # Check health status
  stone osd set-nearfull-ratio .913
  stone health -f json | grep OSD_OUT_OF_ORDER_FULL
  stone health detail | grep OSD_OUT_OF_ORDER_FULL
  stone osd set-nearfull-ratio .892
  stone osd set-backfillfull-ratio .963
  stone health -f json | grep OSD_OUT_OF_ORDER_FULL
  stone health detail | grep OSD_OUT_OF_ORDER_FULL
  stone osd set-backfillfull-ratio .912

  # Check injected full results
  $SUDO stone tell osd.0 injectfull nearfull
  wait_for_health "OSD_NEARFULL"
  stone health detail | grep "osd.0 is near full"
  $SUDO stone tell osd.0 injectfull none
  wait_for_health_ok

  $SUDO stone tell osd.1 injectfull backfillfull
  wait_for_health "OSD_BACKFILLFULL"
  stone health detail | grep "osd.1 is backfill full"
  $SUDO stone tell osd.1 injectfull none
  wait_for_health_ok

  $SUDO stone tell osd.2 injectfull failsafe
  # failsafe and full are the same as far as the monitor is concerned
  wait_for_health "OSD_FULL"
  stone health detail | grep "osd.2 is full"
  $SUDO stone tell osd.2 injectfull none
  wait_for_health_ok

  $SUDO stone tell osd.0 injectfull full
  wait_for_health "OSD_FULL"
  stone health detail | grep "osd.0 is full"
  $SUDO stone tell osd.0 injectfull none
  wait_for_health_ok

  stone pg stat | grep 'pgs:'
  stone pg 1.0 query
  stone tell 1.0 query
  first=$(stone mon dump -f json | jq -r '.mons[0].name')
  stone tell mon.$first quorum enter
  stone quorum_status
  stone report | grep osd_stats
  stone status
  stone -s

  #
  # tell osd version
  #
  stone tell osd.0 version
  expect_false stone tell osd.9999 version 
  expect_false stone tell osd.foo version

  # back to pg stuff

  stone tell osd.0 dump_pg_recovery_stats | grep Started

  stone osd reweight 0 0.9
  expect_false stone osd reweight 0 -1
  stone osd reweight osd.0 1

  stone osd primary-affinity osd.0 .9
  expect_false stone osd primary-affinity osd.0 -2
  expect_false stone osd primary-affinity osd.9999 .5
  stone osd primary-affinity osd.0 1

  stone osd pool set rbd size 2
  stone osd pg-temp 1.0 0 1
  stone osd pg-temp 1.0 osd.1 osd.0
  expect_false stone osd pg-temp 1.0 0 1 2
  expect_false stone osd pg-temp asdf qwer
  expect_false stone osd pg-temp 1.0 asdf
  stone osd pg-temp 1.0 # cleanup pg-temp

  stone pg repeer 1.0
  expect_false stone pg repeer 0.0   # pool 0 shouldn't exist anymore

  # don't test stone osd primary-temp for now
}

function test_mon_osd_pool_set()
{
  TEST_POOL_GETSET=pool_getset
  expect_false stone osd pool create $TEST_POOL_GETSET 1 --target_size_ratio -0.3
  expect_true stone osd pool create $TEST_POOL_GETSET 1 --target_size_ratio 1
  stone osd pool application enable $TEST_POOL_GETSET rados
  stone osd pool set $TEST_POOL_GETSET pg_autoscale_mode off
  wait_for_clean
  stone osd pool get $TEST_POOL_GETSET all

  for s in pg_num pgp_num size min_size crush_rule target_size_ratio; do
    stone osd pool get $TEST_POOL_GETSET $s
  done

  old_size=$(stone osd pool get $TEST_POOL_GETSET size | sed -e 's/size: //')
  (( new_size = old_size + 1 ))
  stone osd pool set $TEST_POOL_GETSET size $new_size --yes-i-really-mean-it
  stone osd pool get $TEST_POOL_GETSET size | grep "size: $new_size"
  stone osd pool set $TEST_POOL_GETSET size $old_size --yes-i-really-mean-it

  stone osd pool create pool_erasure 1 1 erasure
  stone osd pool application enable pool_erasure rados
  wait_for_clean
  set +e
  stone osd pool set pool_erasure size 4444 2>$TMPFILE
  check_response 'not change the size'
  set -e
  stone osd pool get pool_erasure erasure_code_profile
  stone osd pool rm pool_erasure pool_erasure --yes-i-really-really-mean-it

  for flag in nodelete nopgchange nosizechange write_fadvise_dontneed noscrub nodeep-scrub bulk; do
      stone osd pool set $TEST_POOL_GETSET $flag false
      stone osd pool get $TEST_POOL_GETSET $flag | grep "$flag: false"
      stone osd pool set $TEST_POOL_GETSET $flag true
      stone osd pool get $TEST_POOL_GETSET $flag | grep "$flag: true"
      stone osd pool set $TEST_POOL_GETSET $flag 1
      stone osd pool get $TEST_POOL_GETSET $flag | grep "$flag: true"
      stone osd pool set $TEST_POOL_GETSET $flag 0
      stone osd pool get $TEST_POOL_GETSET $flag | grep "$flag: false"
      expect_false stone osd pool set $TEST_POOL_GETSET $flag asdf
      expect_false stone osd pool set $TEST_POOL_GETSET $flag 2
  done

  stone osd pool get $TEST_POOL_GETSET scrub_min_interval | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET scrub_min_interval 123456
  stone osd pool get $TEST_POOL_GETSET scrub_min_interval | grep 'scrub_min_interval: 123456'
  stone osd pool set $TEST_POOL_GETSET scrub_min_interval 0
  stone osd pool get $TEST_POOL_GETSET scrub_min_interval | expect_false grep '.'

  stone osd pool get $TEST_POOL_GETSET scrub_max_interval | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET scrub_max_interval 123456
  stone osd pool get $TEST_POOL_GETSET scrub_max_interval | grep 'scrub_max_interval: 123456'
  stone osd pool set $TEST_POOL_GETSET scrub_max_interval 0
  stone osd pool get $TEST_POOL_GETSET scrub_max_interval | expect_false grep '.'

  stone osd pool get $TEST_POOL_GETSET deep_scrub_interval | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET deep_scrub_interval 123456
  stone osd pool get $TEST_POOL_GETSET deep_scrub_interval | grep 'deep_scrub_interval: 123456'
  stone osd pool set $TEST_POOL_GETSET deep_scrub_interval 0
  stone osd pool get $TEST_POOL_GETSET deep_scrub_interval | expect_false grep '.'

  stone osd pool get $TEST_POOL_GETSET recovery_priority | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET recovery_priority 5 
  stone osd pool get $TEST_POOL_GETSET recovery_priority | grep 'recovery_priority: 5'
  stone osd pool set $TEST_POOL_GETSET recovery_priority -5
  stone osd pool get $TEST_POOL_GETSET recovery_priority | grep 'recovery_priority: -5'
  stone osd pool set $TEST_POOL_GETSET recovery_priority 0
  stone osd pool get $TEST_POOL_GETSET recovery_priority | expect_false grep '.'
  expect_false stone osd pool set $TEST_POOL_GETSET recovery_priority -11
  expect_false stone osd pool set $TEST_POOL_GETSET recovery_priority 11

  stone osd pool get $TEST_POOL_GETSET recovery_op_priority | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET recovery_op_priority 5 
  stone osd pool get $TEST_POOL_GETSET recovery_op_priority | grep 'recovery_op_priority: 5'
  stone osd pool set $TEST_POOL_GETSET recovery_op_priority 0
  stone osd pool get $TEST_POOL_GETSET recovery_op_priority | expect_false grep '.'

  stone osd pool get $TEST_POOL_GETSET scrub_priority | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET scrub_priority 5 
  stone osd pool get $TEST_POOL_GETSET scrub_priority | grep 'scrub_priority: 5'
  stone osd pool set $TEST_POOL_GETSET scrub_priority 0
  stone osd pool get $TEST_POOL_GETSET scrub_priority | expect_false grep '.'

  expect_false stone osd pool set $TEST_POOL_GETSET target_size_ratio -3
  expect_false stone osd pool set $TEST_POOL_GETSET target_size_ratio abc
  expect_true stone osd pool set $TEST_POOL_GETSET target_size_ratio 0.1
  expect_true stone osd pool set $TEST_POOL_GETSET target_size_ratio 1
  stone osd pool get $TEST_POOL_GETSET target_size_ratio | grep 'target_size_ratio: 1'

  stone osd pool set $TEST_POOL_GETSET nopgchange 1
  expect_false stone osd pool set $TEST_POOL_GETSET pg_num 10
  expect_false stone osd pool set $TEST_POOL_GETSET pgp_num 10
  stone osd pool set $TEST_POOL_GETSET nopgchange 0
  stone osd pool set $TEST_POOL_GETSET pg_num 10
  wait_for_clean
  stone osd pool set $TEST_POOL_GETSET pgp_num 10
  expect_false stone osd pool set $TEST_POOL_GETSET pg_num 0
  expect_false stone osd pool set $TEST_POOL_GETSET pgp_num 0

  old_pgs=$(stone osd pool get $TEST_POOL_GETSET pg_num | sed -e 's/pg_num: //')
  new_pgs=$(($old_pgs + $(stone osd stat --format json | jq '.num_osds') * 32))
  stone osd pool set $TEST_POOL_GETSET pg_num $new_pgs
  stone osd pool set $TEST_POOL_GETSET pgp_num $new_pgs
  wait_for_clean

  stone osd pool set $TEST_POOL_GETSET nosizechange 1
  expect_false stone osd pool set $TEST_POOL_GETSET size 2
  expect_false stone osd pool set $TEST_POOL_GETSET min_size 2
  stone osd pool set $TEST_POOL_GETSET nosizechange 0
  stone osd pool set $TEST_POOL_GETSET size 2
  wait_for_clean
  stone osd pool set $TEST_POOL_GETSET min_size 2
  
  expect_false stone osd pool set $TEST_POOL_GETSET hashpspool 0
  stone osd pool set $TEST_POOL_GETSET hashpspool 0 --yes-i-really-mean-it
  
  expect_false stone osd pool set $TEST_POOL_GETSET hashpspool 1
  stone osd pool set $TEST_POOL_GETSET hashpspool 1 --yes-i-really-mean-it

  stone osd pool get rbd crush_rule | grep 'crush_rule: '

  stone osd pool get $TEST_POOL_GETSET compression_mode | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET compression_mode aggressive
  stone osd pool get $TEST_POOL_GETSET compression_mode | grep 'aggressive'
  stone osd pool set $TEST_POOL_GETSET compression_mode unset
  stone osd pool get $TEST_POOL_GETSET compression_mode | expect_false grep '.'

  stone osd pool get $TEST_POOL_GETSET compression_algorithm | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET compression_algorithm zlib
  stone osd pool get $TEST_POOL_GETSET compression_algorithm | grep 'zlib'
  stone osd pool set $TEST_POOL_GETSET compression_algorithm unset
  stone osd pool get $TEST_POOL_GETSET compression_algorithm | expect_false grep '.'

  stone osd pool get $TEST_POOL_GETSET compression_required_ratio | expect_false grep '.'
  expect_false stone osd pool set $TEST_POOL_GETSET compression_required_ratio 1.1
  expect_false stone osd pool set $TEST_POOL_GETSET compression_required_ratio -.2
  stone osd pool set $TEST_POOL_GETSET compression_required_ratio .2
  stone osd pool get $TEST_POOL_GETSET compression_required_ratio | grep '.2'
  stone osd pool set $TEST_POOL_GETSET compression_required_ratio 0
  stone osd pool get $TEST_POOL_GETSET compression_required_ratio | expect_false grep '.'

  stone osd pool get $TEST_POOL_GETSET csum_type | expect_false grep '.'
  stone osd pool set $TEST_POOL_GETSET csum_type crc32c
  stone osd pool get $TEST_POOL_GETSET csum_type | grep 'crc32c'
  stone osd pool set $TEST_POOL_GETSET csum_type unset
  stone osd pool get $TEST_POOL_GETSET csum_type | expect_false grep '.'

  for size in compression_max_blob_size compression_min_blob_size csum_max_block csum_min_block; do
      stone osd pool get $TEST_POOL_GETSET $size | expect_false grep '.'
      stone osd pool set $TEST_POOL_GETSET $size 100
      stone osd pool get $TEST_POOL_GETSET $size | grep '100'
      stone osd pool set $TEST_POOL_GETSET $size 0
      stone osd pool get $TEST_POOL_GETSET $size | expect_false grep '.'
  done

  stone osd pool set $TEST_POOL_GETSET nodelete 1
  expect_false stone osd pool delete $TEST_POOL_GETSET $TEST_POOL_GETSET --yes-i-really-really-mean-it
  stone osd pool set $TEST_POOL_GETSET nodelete 0
  stone osd pool delete $TEST_POOL_GETSET $TEST_POOL_GETSET --yes-i-really-really-mean-it

}

function test_mon_osd_tiered_pool_set()
{
  # this is really a tier pool
  stone osd pool create real-tier 2
  stone osd tier add rbd real-tier

  # expect us to be unable to set negative values for hit_set_*
  for o in hit_set_period hit_set_count hit_set_fpp; do
    expect_false stone osd pool set real_tier $o -1
  done

  # and hit_set_fpp should be in range 0..1
  expect_false stone osd pool set real_tier hit_set_fpp 2

  stone osd pool set real-tier hit_set_type explicit_hash
  stone osd pool get real-tier hit_set_type | grep "hit_set_type: explicit_hash"
  stone osd pool set real-tier hit_set_type explicit_object
  stone osd pool get real-tier hit_set_type | grep "hit_set_type: explicit_object"
  stone osd pool set real-tier hit_set_type bloom
  stone osd pool get real-tier hit_set_type | grep "hit_set_type: bloom"
  expect_false stone osd pool set real-tier hit_set_type i_dont_exist
  stone osd pool set real-tier hit_set_period 123
  stone osd pool get real-tier hit_set_period | grep "hit_set_period: 123"
  stone osd pool set real-tier hit_set_count 12
  stone osd pool get real-tier hit_set_count | grep "hit_set_count: 12"
  stone osd pool set real-tier hit_set_fpp .01
  stone osd pool get real-tier hit_set_fpp | grep "hit_set_fpp: 0.01"

  stone osd pool set real-tier target_max_objects 123
  stone osd pool get real-tier target_max_objects | \
    grep 'target_max_objects:[ \t]\+123'
  stone osd pool set real-tier target_max_bytes 123456
  stone osd pool get real-tier target_max_bytes | \
    grep 'target_max_bytes:[ \t]\+123456'
  stone osd pool set real-tier cache_target_dirty_ratio .123
  stone osd pool get real-tier cache_target_dirty_ratio | \
    grep 'cache_target_dirty_ratio:[ \t]\+0.123'
  expect_false stone osd pool set real-tier cache_target_dirty_ratio -.2
  expect_false stone osd pool set real-tier cache_target_dirty_ratio 1.1
  stone osd pool set real-tier cache_target_dirty_high_ratio .123
  stone osd pool get real-tier cache_target_dirty_high_ratio | \
    grep 'cache_target_dirty_high_ratio:[ \t]\+0.123'
  expect_false stone osd pool set real-tier cache_target_dirty_high_ratio -.2
  expect_false stone osd pool set real-tier cache_target_dirty_high_ratio 1.1
  stone osd pool set real-tier cache_target_full_ratio .123
  stone osd pool get real-tier cache_target_full_ratio | \
    grep 'cache_target_full_ratio:[ \t]\+0.123'
  stone osd dump -f json-pretty | grep '"cache_target_full_ratio_micro": 123000'
  stone osd pool set real-tier cache_target_full_ratio 1.0
  stone osd pool set real-tier cache_target_full_ratio 0
  expect_false stone osd pool set real-tier cache_target_full_ratio 1.1
  stone osd pool set real-tier cache_min_flush_age 123
  stone osd pool get real-tier cache_min_flush_age | \
    grep 'cache_min_flush_age:[ \t]\+123'
  stone osd pool set real-tier cache_min_evict_age 234
  stone osd pool get real-tier cache_min_evict_age | \
    grep 'cache_min_evict_age:[ \t]\+234'

  # iec vs si units
  stone osd pool set real-tier target_max_objects 1K
  stone osd pool get real-tier target_max_objects | grep 1000
  for o in target_max_bytes target_size_bytes compression_max_blob_size compression_min_blob_size csum_max_block csum_min_block; do
    stone osd pool set real-tier $o 1Ki  # no i suffix
    val=$(stone osd pool get real-tier $o --format=json | jq -c ".$o")
    [[ $val  == 1024 ]]
    stone osd pool set real-tier $o 1M   # with i suffix
    val=$(stone osd pool get real-tier $o --format=json | jq -c ".$o")
    [[ $val  == 1048576 ]]
  done

  # this is not a tier pool
  stone osd pool create fake-tier 2
  stone osd pool application enable fake-tier rados
  wait_for_clean

  expect_false stone osd pool set fake-tier hit_set_type explicit_hash
  expect_false stone osd pool get fake-tier hit_set_type
  expect_false stone osd pool set fake-tier hit_set_type explicit_object
  expect_false stone osd pool get fake-tier hit_set_type
  expect_false stone osd pool set fake-tier hit_set_type bloom
  expect_false stone osd pool get fake-tier hit_set_type
  expect_false stone osd pool set fake-tier hit_set_type i_dont_exist
  expect_false stone osd pool set fake-tier hit_set_period 123
  expect_false stone osd pool get fake-tier hit_set_period
  expect_false stone osd pool set fake-tier hit_set_count 12
  expect_false stone osd pool get fake-tier hit_set_count
  expect_false stone osd pool set fake-tier hit_set_fpp .01
  expect_false stone osd pool get fake-tier hit_set_fpp

  expect_false stone osd pool set fake-tier target_max_objects 123
  expect_false stone osd pool get fake-tier target_max_objects
  expect_false stone osd pool set fake-tier target_max_bytes 123456
  expect_false stone osd pool get fake-tier target_max_bytes
  expect_false stone osd pool set fake-tier cache_target_dirty_ratio .123
  expect_false stone osd pool get fake-tier cache_target_dirty_ratio
  expect_false stone osd pool set fake-tier cache_target_dirty_ratio -.2
  expect_false stone osd pool set fake-tier cache_target_dirty_ratio 1.1
  expect_false stone osd pool set fake-tier cache_target_dirty_high_ratio .123
  expect_false stone osd pool get fake-tier cache_target_dirty_high_ratio
  expect_false stone osd pool set fake-tier cache_target_dirty_high_ratio -.2
  expect_false stone osd pool set fake-tier cache_target_dirty_high_ratio 1.1
  expect_false stone osd pool set fake-tier cache_target_full_ratio .123
  expect_false stone osd pool get fake-tier cache_target_full_ratio
  expect_false stone osd pool set fake-tier cache_target_full_ratio 1.0
  expect_false stone osd pool set fake-tier cache_target_full_ratio 0
  expect_false stone osd pool set fake-tier cache_target_full_ratio 1.1
  expect_false stone osd pool set fake-tier cache_min_flush_age 123
  expect_false stone osd pool get fake-tier cache_min_flush_age
  expect_false stone osd pool set fake-tier cache_min_evict_age 234
  expect_false stone osd pool get fake-tier cache_min_evict_age

  stone osd tier remove rbd real-tier
  stone osd pool delete real-tier real-tier --yes-i-really-really-mean-it
  stone osd pool delete fake-tier fake-tier --yes-i-really-really-mean-it
}

function test_mon_osd_erasure_code()
{

  stone osd erasure-code-profile set fooprofile a=b c=d
  stone osd erasure-code-profile set fooprofile a=b c=d
  expect_false stone osd erasure-code-profile set fooprofile a=b c=d e=f
  stone osd erasure-code-profile set fooprofile a=b c=d e=f --force
  stone osd erasure-code-profile set fooprofile a=b c=d e=f
  expect_false stone osd erasure-code-profile set fooprofile a=b c=d e=f g=h
  # make sure ruleset-foo doesn't work anymore
  expect_false stone osd erasure-code-profile set barprofile ruleset-failure-domain=host
  stone osd erasure-code-profile set barprofile crush-failure-domain=host
  # clean up
  stone osd erasure-code-profile rm fooprofile
  stone osd erasure-code-profile rm barprofile

  # try weird k and m values
  expect_false stone osd erasure-code-profile set badk k=1 m=1
  expect_false stone osd erasure-code-profile set badk k=1 m=2
  expect_false stone osd erasure-code-profile set badk k=0 m=2
  expect_false stone osd erasure-code-profile set badk k=-1 m=2
  expect_false stone osd erasure-code-profile set badm k=2 m=0
  expect_false stone osd erasure-code-profile set badm k=2 m=-1
  stone osd erasure-code-profile set good k=2 m=1
  stone osd erasure-code-profile rm good
}

function test_mon_osd_misc()
{
  set +e

  # expect error about missing 'pool' argument
  stone osd map 2>$TMPFILE; check_response 'pool' $? 22

  # expect error about unused argument foo
  stone osd ls foo 2>$TMPFILE; check_response 'unused' $? 22 

  # expect "not in range" for invalid overload percentage
  stone osd reweight-by-utilization 80 2>$TMPFILE; check_response 'higher than 100' $? 22

  set -e

  local old_bytes_per_osd=$(stone config get mgr mon_reweight_min_bytes_per_osd)
  local old_pgs_per_osd=$(stone config get mgr mon_reweight_min_pgs_per_osd)
  # otherwise stone-mgr complains like:
  # Error EDOM: Refusing to reweight: we only have 5372 kb used across all osds!
  # Error EDOM: Refusing to reweight: we only have 20 PGs across 3 osds!
  stone config set mgr mon_reweight_min_bytes_per_osd 0
  stone config set mgr mon_reweight_min_pgs_per_osd 0
  stone osd reweight-by-utilization 110
  stone osd reweight-by-utilization 110 .5
  expect_false stone osd reweight-by-utilization 110 0
  expect_false stone osd reweight-by-utilization 110 -0.1
  stone osd test-reweight-by-utilization 110 .5 --no-increasing
  stone osd test-reweight-by-utilization 110 .5 4 --no-increasing
  expect_false stone osd test-reweight-by-utilization 110 .5 0 --no-increasing
  expect_false stone osd test-reweight-by-utilization 110 .5 -10 --no-increasing
  stone osd reweight-by-pg 110
  stone osd test-reweight-by-pg 110 .5
  stone osd reweight-by-pg 110 rbd
  stone osd reweight-by-pg 110 .5 rbd
  expect_false stone osd reweight-by-pg 110 boguspoolasdfasdfasdf
  # restore the setting
  stone config set mgr mon_reweight_min_bytes_per_osd $old_bytes_per_osd
  stone config set mgr mon_reweight_min_pgs_per_osd $old_pgs_per_osd
}

function test_admin_heap_profiler()
{
  do_test=1
  set +e
  # expect 'heap' commands to be correctly parsed
  stone tell osd.0 heap stats 2>$TMPFILE
  if [[ $? -eq 22 && `grep 'tcmalloc not enabled' $TMPFILE` ]]; then
    echo "tcmalloc not enabled; skip heap profiler test"
    do_test=0
  fi
  set -e

  [[ $do_test -eq 0 ]] && return 0

  $SUDO stone tell osd.0 heap start_profiler
  $SUDO stone tell osd.0 heap dump
  $SUDO stone tell osd.0 heap stop_profiler
  $SUDO stone tell osd.0 heap release
}

function test_osd_bench()
{
  # test osd bench limits
  # As we should not rely on defaults (as they may change over time),
  # lets inject some values and perform some simple tests
  # max iops: 10              # 100 IOPS
  # max throughput: 10485760  # 10MB/s
  # max block size: 2097152   # 2MB
  # duration: 10              # 10 seconds

  local args="\
    --osd-bench-duration 10 \
    --osd-bench-max-block-size 2097152 \
    --osd-bench-large-size-max-throughput 10485760 \
    --osd-bench-small-size-max-iops 10"
  stone tell osd.0 injectargs ${args## }

  # anything with a bs larger than 2097152  must fail
  expect_false stone tell osd.0 bench 1 2097153
  # but using 'osd_bench_max_bs' must succeed
  stone tell osd.0 bench 1 2097152

  # we assume 1MB as a large bs; anything lower is a small bs
  # for a 4096 bytes bs, for 10 seconds, we are limited by IOPS
  # max count: 409600 (bytes)

  # more than max count must not be allowed
  expect_false stone tell osd.0 bench 409601 4096
  # but 409600 must be succeed
  stone tell osd.0 bench 409600 4096

  # for a large bs, we are limited by throughput.
  # for a 2MB block size for 10 seconds, assuming 10MB/s throughput,
  # the max count will be (10MB * 10s) = 100MB
  # max count: 104857600 (bytes)

  # more than max count must not be allowed
  expect_false stone tell osd.0 bench 104857601 2097152
  # up to max count must be allowed
  stone tell osd.0 bench 104857600 2097152
}

function test_osd_negative_filestore_merge_threshold()
{
  $SUDO stone daemon osd.0 config set filestore_merge_threshold -1
  expect_config_value "osd.0" "filestore_merge_threshold" -1
}

function test_mon_tell()
{
  for m in mon.a mon.b; do
    stone tell $m sessions
    stone_watch_start debug audit
    stone tell mon.a sessions
    stone_watch_wait "${m} \[DBG\] from.*cmd='sessions' args=\[\]: dispatch"
  done
  expect_false stone tell mon.foo version
}

function test_mon_ping()
{
  stone ping mon.a
  stone ping mon.b
  expect_false stone ping mon.foo

  stone ping mon.\*
}

function test_mon_deprecated_commands()
{
  # current DEPRECATED commands are marked with FLAG(DEPRECATED)
  #
  # Testing should be accomplished by setting
  # 'mon_debug_deprecated_as_obsolete = true' and expecting ENOTSUP for
  # each one of these commands.

  stone tell mon.* injectargs '--mon-debug-deprecated-as-obsolete'
  expect_false stone config-key list 2> $TMPFILE
  check_response "\(EOPNOTSUPP\|ENOTSUP\): command is obsolete"

  stone tell mon.* injectargs '--no-mon-debug-deprecated-as-obsolete'
}

function test_mon_stonedf_commands()
{
  # stone df detail:
  # pool section:
  # RAW USED The near raw used per pool in raw total

  stone osd pool create stonedf_for_test 1 1 replicated
  stone osd pool application enable stonedf_for_test rados
  stone osd pool set stonedf_for_test size 2

  dd if=/dev/zero of=./stonedf_for_test bs=4k count=1
  rados put stonedf_for_test stonedf_for_test -p stonedf_for_test

  #wait for update
  for i in `seq 1 10`; do
    rados -p stonedf_for_test ls - | grep -q stonedf_for_test && break
    sleep 1
  done
  # "rados ls" goes straight to osd, but "stone df" is served by mon. so we need
  # to sync mon with osd
  flush_pg_stats
  local jq_filter='.pools | .[] | select(.name == "stonedf_for_test") | .stats'
  stored=`stone df detail --format=json | jq "$jq_filter.stored * 2"`
  stored_raw=`stone df detail --format=json | jq "$jq_filter.stored_raw"`

  stone osd pool delete stonedf_for_test stonedf_for_test --yes-i-really-really-mean-it
  rm ./stonedf_for_test

  expect_false test $stored != $stored_raw
}

function test_mon_pool_application()
{
  stone osd pool create app_for_test 16

  stone osd pool application enable app_for_test rbd
  expect_false stone osd pool application enable app_for_test rgw
  stone osd pool application enable app_for_test rgw --yes-i-really-mean-it
  stone osd pool ls detail | grep "application rbd,rgw"
  stone osd pool ls detail --format=json | grep '"application_metadata":{"rbd":{},"rgw":{}}'

  expect_false stone osd pool application set app_for_test stonefs key value
  stone osd pool application set app_for_test rbd key1 value1
  stone osd pool application set app_for_test rbd key2 value2
  stone osd pool application set app_for_test rgw key1 value1
  stone osd pool application get app_for_test rbd key1 | grep 'value1'
  stone osd pool application get app_for_test rbd key2 | grep 'value2'
  stone osd pool application get app_for_test rgw key1 | grep 'value1'

  stone osd pool ls detail --format=json | grep '"application_metadata":{"rbd":{"key1":"value1","key2":"value2"},"rgw":{"key1":"value1"}}'

  stone osd pool application rm app_for_test rgw key1
  stone osd pool ls detail --format=json | grep '"application_metadata":{"rbd":{"key1":"value1","key2":"value2"},"rgw":{}}'
  stone osd pool application rm app_for_test rbd key2
  stone osd pool ls detail --format=json | grep '"application_metadata":{"rbd":{"key1":"value1"},"rgw":{}}'
  stone osd pool application rm app_for_test rbd key1
  stone osd pool ls detail --format=json | grep '"application_metadata":{"rbd":{},"rgw":{}}'
  stone osd pool application rm app_for_test rbd key1 # should be idempotent

  expect_false stone osd pool application disable app_for_test rgw
  stone osd pool application disable app_for_test rgw --yes-i-really-mean-it
  stone osd pool application disable app_for_test rgw --yes-i-really-mean-it # should be idempotent
  stone osd pool ls detail | grep "application rbd"
  stone osd pool ls detail --format=json | grep '"application_metadata":{"rbd":{}}'

  stone osd pool application disable app_for_test rgw --yes-i-really-mean-it
  stone osd pool ls detail | grep -v "application "
  stone osd pool ls detail --format=json | grep '"application_metadata":{}'

  stone osd pool rm app_for_test app_for_test --yes-i-really-really-mean-it
}

function test_mon_tell_help_command()
{
  stone tell mon.a help | grep sync_force
  stone tell mon.a -h | grep sync_force
  stone tell mon.a config -h | grep 'config diff get'

  # wrong target
  expect_false stone tell mon.zzz help
}

function test_mon_stdin_stdout()
{
  echo foo | stone config-key set test_key -i -
  stone config-key get test_key -o - | grep -c foo | grep -q 1
}

function test_osd_tell_help_command()
{
  stone tell osd.1 help
  expect_false stone tell osd.100 help
}

function test_osd_compact()
{
  stone tell osd.1 compact
  $SUDO stone daemon osd.1 compact
}

function test_mds_tell_help_command()
{
  local FS_NAME=stonefs
  if ! mds_exists ; then
      echo "Skipping test, no MDS found"
      return
  fi

  remove_all_fs
  stone osd pool create fs_data 16
  stone osd pool create fs_metadata 16
  stone fs new $FS_NAME fs_metadata fs_data
  wait_mds_active $FS_NAME


  stone tell mds.a help
  expect_false stone tell mds.z help

  remove_all_fs
  stone osd pool delete fs_data fs_data --yes-i-really-really-mean-it
  stone osd pool delete fs_metadata fs_metadata --yes-i-really-really-mean-it
}

function test_mgr_tell()
{
  stone tell mgr version
}

function test_mgr_devices()
{
  stone device ls
  expect_false stone device info doesnotexist
  expect_false stone device get-health-metrics doesnotexist
}

function test_per_pool_scrub_status()
{
  stone osd pool create noscrub_pool 16
  stone osd pool create noscrub_pool2 16
  stone -s | expect_false grep -q "Some pool(s) have the.*scrub.* flag(s) set"
  stone -s --format json | \
    jq .health.checks.POOL_SCRUB_FLAGS.summary.message | \
    expect_false grep -q "Some pool(s) have the.*scrub.* flag(s) set"
  stone report | jq .health.checks.POOL_SCRUB_FLAGS.detail |
    expect_false grep -q "Pool .* has .*scrub.* flag"
  stone health detail | jq .health.checks.POOL_SCRUB_FLAGS.detail | \
    expect_false grep -q "Pool .* has .*scrub.* flag"

  stone osd pool set noscrub_pool noscrub 1
  stone -s | expect_true grep -q "Some pool(s) have the noscrub flag(s) set"
  stone -s --format json | \
    jq .health.checks.POOL_SCRUB_FLAGS.summary.message | \
    expect_true grep -q "Some pool(s) have the noscrub flag(s) set"
  stone report | jq .health.checks.POOL_SCRUB_FLAGS.detail | \
    expect_true grep -q "Pool noscrub_pool has noscrub flag"
  stone health detail | expect_true grep -q "Pool noscrub_pool has noscrub flag"

  stone osd pool set noscrub_pool nodeep-scrub 1
  stone osd pool set noscrub_pool2 nodeep-scrub 1
  stone -s | expect_true grep -q "Some pool(s) have the noscrub, nodeep-scrub flag(s) set"
  stone -s --format json | \
    jq .health.checks.POOL_SCRUB_FLAGS.summary.message | \
    expect_true grep -q "Some pool(s) have the noscrub, nodeep-scrub flag(s) set"
  stone report | jq .health.checks.POOL_SCRUB_FLAGS.detail | \
    expect_true grep -q "Pool noscrub_pool has noscrub flag"
  stone report | jq .health.checks.POOL_SCRUB_FLAGS.detail | \
    expect_true grep -q "Pool noscrub_pool has nodeep-scrub flag"
  stone report | jq .health.checks.POOL_SCRUB_FLAGS.detail | \
    expect_true grep -q "Pool noscrub_pool2 has nodeep-scrub flag"
  stone health detail | expect_true grep -q "Pool noscrub_pool has noscrub flag"
  stone health detail | expect_true grep -q "Pool noscrub_pool has nodeep-scrub flag"
  stone health detail | expect_true grep -q "Pool noscrub_pool2 has nodeep-scrub flag"

  stone osd pool rm noscrub_pool noscrub_pool --yes-i-really-really-mean-it
  stone osd pool rm noscrub_pool2 noscrub_pool2 --yes-i-really-really-mean-it
}

#
# New tests should be added to the TESTS array below
#
# Individual tests may be run using the '-t <testname>' argument
# The user can specify '-t <testname>' as many times as she wants
#
# Tests will be run in order presented in the TESTS array, or in
# the order specified by the '-t <testname>' options.
#
# '-l' will list all the available test names
# '-h' will show usage
#
# The test maintains backward compatibility: not specifying arguments
# will run all tests following the order they appear in the TESTS array.
#

set +x
MON_TESTS+=" mon_injectargs"
MON_TESTS+=" mon_injectargs_SI"
for i in `seq 9`; do
    MON_TESTS+=" tiering_$i";
done
MON_TESTS+=" auth"
MON_TESTS+=" auth_profiles"
MON_TESTS+=" mon_misc"
MON_TESTS+=" mon_mon"
MON_TESTS+=" mon_osd"
MON_TESTS+=" mon_config_key"
MON_TESTS+=" mon_crush"
MON_TESTS+=" mon_osd_create_destroy"
MON_TESTS+=" mon_osd_pool"
MON_TESTS+=" mon_osd_pool_quota"
MON_TESTS+=" mon_pg"
MON_TESTS+=" mon_osd_pool_set"
MON_TESTS+=" mon_osd_tiered_pool_set"
MON_TESTS+=" mon_osd_erasure_code"
MON_TESTS+=" mon_osd_misc"
MON_TESTS+=" mon_tell"
MON_TESTS+=" mon_ping"
MON_TESTS+=" mon_deprecated_commands"
MON_TESTS+=" mon_caps"
MON_TESTS+=" mon_stonedf_commands"
MON_TESTS+=" mon_tell_help_command"
MON_TESTS+=" mon_stdin_stdout"

OSD_TESTS+=" osd_bench"
OSD_TESTS+=" osd_negative_filestore_merge_threshold"
OSD_TESTS+=" tiering_agent"
OSD_TESTS+=" admin_heap_profiler"
OSD_TESTS+=" osd_tell_help_command"
OSD_TESTS+=" osd_compact"
OSD_TESTS+=" per_pool_scrub_status"

MDS_TESTS+=" mds_tell"
MDS_TESTS+=" mon_mds"
MDS_TESTS+=" mon_mds_metadata"
MDS_TESTS+=" mds_tell_help_command"

MGR_TESTS+=" mgr_tell"
MGR_TESTS+=" mgr_devices"

TESTS+=$MON_TESTS
TESTS+=$OSD_TESTS
TESTS+=$MDS_TESTS
TESTS+=$MGR_TESTS

#
# "main" follows
#

function list_tests()
{
  echo "AVAILABLE TESTS"
  for i in $TESTS; do
    echo "  $i"
  done
}

function usage()
{
  echo "usage: $0 [-h|-l|-t <testname> [-t <testname>...]]"
}

tests_to_run=()

sanity_check=true

while [[ $# -gt 0 ]]; do
  opt=$1

  case "$opt" in
    "-l" )
      do_list=1
      ;;
    "--asok-does-not-need-root" )
      SUDO=""
      ;;
    "--no-sanity-check" )
      sanity_check=false
      ;;
    "--test-mon" )
      tests_to_run+="$MON_TESTS"
      ;;
    "--test-osd" )
      tests_to_run+="$OSD_TESTS"
      ;;
    "--test-mds" )
      tests_to_run+="$MDS_TESTS"
      ;;
    "--test-mgr" )
      tests_to_run+="$MGR_TESTS"
      ;;
    "-t" )
      shift
      if [[ -z "$1" ]]; then
        echo "missing argument to '-t'"
        usage ;
        exit 1
      fi
      tests_to_run+=" $1"
      ;;
    "-h" )
      usage ;
      exit 0
      ;;
  esac
  shift
done

if [[ $do_list -eq 1 ]]; then
  list_tests ;
  exit 0
fi

stone osd pool create rbd 16

if test -z "$tests_to_run" ; then
  tests_to_run="$TESTS"
fi

if $sanity_check ; then
    wait_no_osd_down
fi
for i in $tests_to_run; do
  if $sanity_check ; then
      check_no_osd_down
  fi
  set -x
  test_${i}
  set +x
done
if $sanity_check ; then
    check_no_osd_down
fi

set -x

echo OK
