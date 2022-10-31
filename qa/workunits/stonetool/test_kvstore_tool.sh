#!/usr/bin/env bash

set -x

source $(dirname $0)/../../standalone/stone-helpers.sh

set -e
set -o functrace
PS4='${BASH_SOURCE[0]}:$LINENO: ${FUNCNAME[0]}:  '
SUDO=${SUDO:-sudo}
export STONE_DEV=1

echo note: test stone_kvstore_tool with bluestore

expect_false()
{
    set -x
    if "$@"; then return 1; else return 0; fi
}

TEMP_DIR=$(mktemp -d ./stonetool.XXX)
trap "rm -fr $TEMP_DIR" 0

TEMP_FILE=$(mktemp $TEMP_DIR/test_invalid.XXX)

function test_stone_kvstore_tool()
{
  # create a data directory
  stone-objectstore-tool --data-path ${TEMP_DIR} --op mkfs --no-mon-config

  # list
  origin_kv_nums=`stone-kvstore-tool  bluestore-kv ${TEMP_DIR} list 2>/dev/null | wc -l`
  
  # exists
  prefix=`stone-kvstore-tool bluestore-kv ${TEMP_DIR} list 2>/dev/null | head -n 1 | awk '{print $1}'`
  stone-kvstore-tool bluestore-kv ${TEMP_DIR} exists ${prefix}
  expect_false stone-kvstore-tool bluestore-kv ${TEMP_DIR} exists ${prefix}notexist

  # list-crc
  stone-kvstore-tool  bluestore-kv ${TEMP_DIR} list-crc
  stone-kvstore-tool  bluestore-kv ${TEMP_DIR} list-crc ${prefix}

  # list with prefix
  stone-kvstore-tool  bluestore-kv ${TEMP_DIR} list ${prefix}

  # set
  echo "helloworld" >> ${TEMP_FILE}
  stone-kvstore-tool bluestore-kv ${TEMP_DIR} set TESTPREFIX TESTKEY in ${TEMP_FILE}
  stone-kvstore-tool bluestore-kv ${TEMP_DIR} exists TESTPREFIX TESTKEY

  # get 
  stone-kvstore-tool bluestore-kv ${TEMP_DIR} get TESTPREFIX TESTKEY out ${TEMP_FILE}.bak
  diff ${TEMP_FILE} ${TEMP_FILE}.bak

  # rm 
  stone-kvstore-tool bluestore-kv ${TEMP_DIR} rm TESTPREFIX TESTKEY
  expect_false stone-kvstore-tool bluestore-kv ${TEMP_DIR} exists TESTPREFIX TESTKEY

  # compact
  stone-kvstore-tool bluestore-kv ${TEMP_DIR} compact

  # destructive-repair 
  stone-kvstore-tool bluestore-kv ${TEMP_DIR} destructive-repair 

  current_kv_nums=`stone-kvstore-tool  bluestore-kv ${TEMP_DIR} list 2>/dev/null | wc -l`
  test ${origin_kv_nums} -eq ${current_kv_nums}
} 

test_stone_kvstore_tool

echo OK
