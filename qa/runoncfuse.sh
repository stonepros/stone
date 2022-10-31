#!/usr/bin/env bash
set -x

mkdir -p testspace
stone-fuse testspace -m $1

./runallonce.sh testspace
killall stone-fuse
