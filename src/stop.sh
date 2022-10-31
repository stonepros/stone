#!/usr/bin/env bash
#
# Copyright (C) 2013 Inktank <info@inktank.com>
# Copyright (C) 2013 Cloudwatt <libre.licensing@cloudwatt.com>
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

test -d dev/osd0/. && test -e dev/sudo && SUDO="sudo"

if [ -e CMakeCache.txt ]; then
  [ -z "$STONE_BIN" ] && STONE_BIN=bin
fi

if [ -n "$VSTART_DEST" ]; then
  STONE_CONF_PATH=$VSTART_DEST
else
  STONE_CONF_PATH="$PWD"
fi
conf_fn="$STONE_CONF_PATH/stone.conf"
STONEADM_DIR_PATH="$STONE_CONF_PATH/../src/stoneadm"

MYUID=$(id -u)
MYNAME=$(id -nu)

do_killall() {
    local pname="stone-run.*$1"
    if [ $1 == "ganesha.nfsd" ]; then
	    pname=$1
    fi
    pg=`pgrep -u $MYUID -f $pname`
    [ -n "$pg" ] && kill $pg
    $SUDO killall -u $MYNAME $1
}

do_killstoneadm() {
    FSID=$($STONE_BIN/stone -c $conf_fn fsid)
    sudo $STONEADM_DIR_PATH/stoneadm rm-cluster --fsid $FSID --force
}

do_umountall() {
    #VSTART_IP_PORTS is of the format as below
    #"[v[num]:IP:PORT/0,v[num]:IP:PORT/0][v[num]:IP:PORT/0,v[num]:IP:PORT/0]..."
    VSTART_IP_PORTS=$("${STONE_BIN}"/stone -c $conf_fn mon metadata 2>/dev/null | jq -j '.[].addrs')

    #SRC_MNT_ARRAY is of the format as below
    #SRC_MNT_ARRAY[0] = IP:PORT,IP:PORT,IP:PORT:/
    #SRC_MNT_ARRAY[1] = MNT_POINT1
    #SRC_MNT_ARRAY[2] = IP:PORT:/ #Could be mounted using single mon IP
    #SRC_MNT_ARRAY[3] = MNT_POINT2
    #...
    SRC_MNT_ARRAY=($(findmnt -t stone -n --raw --output=source,target))
    LEN_SRC_MNT_ARRAY=${#SRC_MNT_ARRAY[@]}

    for (( i=0; i<${LEN_SRC_MNT_ARRAY}; i=$((i+2)) ))
    do
      # The first IP:PORT among the list is checked against vstart monitor IP:PORTS
      IP_PORT1=$(echo ${SRC_MNT_ARRAY[$i]} | awk -F ':/' '{print $1}' | awk -F ',' '{print $1}')
      if [[ "$VSTART_IP_PORTS" == *"$IP_PORT1"* ]]
      then
        STONE_MNT=${SRC_MNT_ARRAY[$((i+1))]}
        [ -n "$STONE_MNT" ] && sudo umount -f $STONE_MNT
      fi
    done

    #Get fuse mounts of the cluster
    STONE_FUSE_MNTS=$("${STONE_BIN}"/stone -c $conf_fn tell mds.* client ls 2>/dev/null | grep mount_point | tr -d '",' | awk '{print $2}')
    [ -n "$STONE_FUSE_MNTS" ] && sudo umount -f $STONE_FUSE_MNTS
}

usage="usage: $0 [all] [mon] [mds] [osd] [rgw] [nfs] [--crimson] [--stoneadm]\n"

stop_all=1
stop_mon=0
stop_mds=0
stop_osd=0
stop_mgr=0
stop_rgw=0
stop_ganesha=0
stone_osd=stone-osd
stop_stoneadm=0

while [ $# -ge 1 ]; do
    case $1 in
        all )
            stop_all=1
            ;;
        mon | stone-mon )
            stop_mon=1
            stop_all=0
            ;;
        mgr | stone-mgr )
            stop_mgr=1
            stop_all=0
            ;;
        mds | stone-mds )
            stop_mds=1
            stop_all=0
            ;;
        osd | stone-osd )
            stop_osd=1
            stop_all=0
            ;;
        rgw | stone-rgw )
            stop_rgw=1
            stop_all=0
            ;;
        nfs | ganesha.nfsd )
            stop_ganesha=1
            stop_all=0
            ;;
        --crimson)
            stone_osd=crimson-osd
            ;;
        --stoneadm)
            stop_stoneadm=1
            stop_all=0
            ;;
        * )
            printf "$usage"
            exit
    esac
    shift
done

if [ $stop_all -eq 1 ]; then
    if "${STONE_BIN}"/stone -s --connect-timeout 1 -c $conf_fn >/dev/null 2>&1; then
        # Umount mounted filesystems from vstart cluster
        do_umountall
    fi

    if "${STONE_BIN}"/rbd device list -c $conf_fn >/dev/null 2>&1; then
        "${STONE_BIN}"/rbd device list -c $conf_fn | tail -n +2 |
        while read DEV; do
            # While it is currently possible to create an rbd image with
            # whitespace chars in its name, krbd will refuse mapping such
            # an image, so we can safely split on whitespace here.  (The
            # same goes for whitespace chars in names of the pools that
            # contain rbd images).
            DEV="$(echo "${DEV}" | tr -s '[:space:]' | awk '{ print $5 }')"
            sudo "${STONE_BIN}"/rbd device unmap "${DEV}" -c $conf_fn
        done

        if [ -n "$("${STONE_BIN}"/rbd device list -c $conf_fn)" ]; then
            echo "WARNING: Some rbd images are still mapped!" >&2
        fi
    fi

    daemons="$($STONEADM_DIR_PATH/stoneadm ls 2> /dev/null)"
    if [ $? -eq 0 -a "$daemons" != "[]" ]; then
        do_killstoneadm
    fi

    for p in $stone_osd stone-mon stone-mds stone-mgr radosgw lt-radosgw apache2 ganesha.nfsd ; do
        for try in 0 1 1 1 1 ; do
            if ! pkill -u $MYUID $p ; then
                break
            fi
            sleep $try
        done
    done

    pkill -u $MYUID -f valgrind.bin.\*stone-mon
    $SUDO pkill -u $MYUID -f valgrind.bin.\*$stone_osd
    pkill -u $MYUID -f valgrind.bin.\*stone-mds
    asok_dir=`dirname $("${STONE_BIN}"/stone-conf -c ${conf_fn} --show-config-value admin_socket)`
    rm -rf "${asok_dir}"
else
    [ $stop_mon -eq 1 ] && do_killall stone-mon
    [ $stop_mds -eq 1 ] && do_killall stone-mds
    [ $stop_osd -eq 1 ] && do_killall $stone_osd
    [ $stop_mgr -eq 1 ] && do_killall stone-mgr
    [ $stop_ganesha -eq 1 ] && do_killall ganesha.nfsd
    [ $stop_rgw -eq 1 ] && do_killall radosgw lt-radosgw apache2
    [ $stop_stoneadm -eq 1 ] && do_killstoneadm
fi
