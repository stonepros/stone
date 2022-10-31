#!/usr/bin/env bash
set -x

mkdir -p testspace
/bin/mount -t stone $1 testspace

./runallonce.sh testspace

/bin/umount testspace
