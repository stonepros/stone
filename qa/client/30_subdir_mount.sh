#!/usr/bin/env bash
set -x

basedir=`echo $0 | sed 's/[^/]*$//g'`.
. $basedir/common.sh

client_mount
mkdir -p $mnt/sub
echo sub > $mnt/sub/file
client_umount

mkdir -p $mnt/1
mkdir -p $mnt/2
/bin/mount -t stone $monhost:/sub $mnt/1
grep sub $mnt/1/file

/bin/mount -t stone $monhost:/ $mnt/2
grep sub $mnt/2/sub/file

/bin/umount $mnt/1
grep sub $mnt/2/sub/file

/bin/umount $mnt/2
