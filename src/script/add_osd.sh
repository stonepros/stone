#!/usr/bin/env bash

set -ex

STONE_DEV_DIR=dev
STONE_BIN=bin
stone_adm=$STONE_BIN/stone
osd=$1
location=$2
weight=.0990

# DANGEROUS
rm -rf $STONE_DEV_DIR/osd$osd
mkdir -p $STONE_DEV_DIR/osd$osd

uuid=`uuidgen`
echo "add osd$osd $uuid"
OSD_SECRET=$($STONE_BIN/stone-authtool --gen-print-key)
echo "{\"stonex_secret\": \"$OSD_SECRET\"}" > $STONE_DEV_DIR/osd$osd/new.json
$STONE_BIN/stone osd new $uuid -i $STONE_DEV_DIR/osd$osd/new.json
rm $STONE_DEV_DIR/osd$osd/new.json
$STONE_BIN/stone-osd -i $osd $ARGS --mkfs --key $OSD_SECRET --osd-uuid $uuid

key_fn=$STONE_DEV_DIR/osd$osd/keyring
cat > $key_fn<<EOF
[osd.$osd]
	key = $OSD_SECRET
EOF
echo adding osd$osd key to auth repository
$STONE_BIN/stone -i "$key_fn" auth add osd.$osd osd "allow *" mon "allow profile osd" mgr "allow profile osd"

$STONE_BIN/stone osd crush add osd.$osd $weight $location

echo start osd.$osd
$STONE_BIN/stone-osd -i $osd $ARGS $COSD_ARGS
