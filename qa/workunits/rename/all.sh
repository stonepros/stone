#!/usr/bin/env bash
set -ex

dir=`dirname $0`

STONE_TOOL='./stone'
$STONE_TOOL || STONE_TOOL='stone'

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/prepare.sh

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/pri_nul.sh
rm ./?/* || true

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/rem_nul.sh
rm ./?/* || true

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/pri_pri.sh
rm ./?/* || true

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/rem_pri.sh
rm ./?/* || true

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/rem_rem.sh
rm ./?/* || true

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/pri_nul.sh
rm -r ./?/* || true

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/pri_pri.sh
rm -r ./?/* || true

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/dir_pri_pri.sh
rm -r ./?/* || true

STONE_ARGS=$STONE_ARGS STONE_TOOL=$STONE_TOOL $dir/dir_pri_nul.sh
rm -r ./?/* || true

