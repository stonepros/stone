#!/usr/bin/env bash
# -*- mode:sh; tab-width:4; sh-basic-offset:4; indent-tabs-mode:nil -*-
# vim: softtabstop=4 shiftwidth=4 expandtab

# abort on failure
set -e

quoted_print() {
    for s in "$@"; do
        if [[ "$s" =~ \  ]]; then
            printf -- "'%s' " "$s"
        else
            printf -- "$s "
        fi
    done
    printf '\n'
}

debug() {
  "$@" >&2
}

prunb() {
    debug quoted_print "$@" '&'
    PATH=$STONE_BIN:$PATH "$@" &
}

prun() {
    debug quoted_print "$@"
    PATH=$STONE_BIN:$PATH "$@"
}


if [ -n "$VSTART_DEST" ]; then
    SRC_PATH=`dirname $0`
    SRC_PATH=`(cd $SRC_PATH; pwd)`

    STONE_DIR=$SRC_PATH
    STONE_BIN=${PWD}/bin
    STONE_LIB=${PWD}/lib

    STONE_CONF_PATH=$VSTART_DEST
    STONE_DEV_DIR=$VSTART_DEST/dev
    STONE_OUT_DIR=$VSTART_DEST/out
    STONE_ASOK_DIR=$VSTART_DEST/out
fi

get_cmake_variable() {
    local variable=$1
    grep "${variable}:" CMakeCache.txt | cut -d "=" -f 2
}

# for running out of the CMake build directory
if [ -e CMakeCache.txt ]; then
    # Out of tree build, learn source location from CMakeCache.txt
    STONE_ROOT=$(get_cmake_variable stone_SOURCE_DIR)
    STONE_BUILD_DIR=`pwd`
    [ -z "$MGR_PYTHON_PATH" ] && MGR_PYTHON_PATH=$STONE_ROOT/src/pybind/mgr
fi

# use STONE_BUILD_ROOT to vstart from a 'make install'
if [ -n "$STONE_BUILD_ROOT" ]; then
    [ -z "$STONE_BIN" ] && STONE_BIN=$STONE_BUILD_ROOT/bin
    [ -z "$STONE_LIB" ] && STONE_LIB=$STONE_BUILD_ROOT/lib
    [ -z "$STONE_EXT_LIB" ] && STONE_EXT_LIB=$STONE_BUILD_ROOT/external/lib
    [ -z "$EC_PATH" ] && EC_PATH=$STONE_LIB/erasure-code
    [ -z "$OBJCLASS_PATH" ] && OBJCLASS_PATH=$STONE_LIB/rados-classes
    # make install should install python extensions into PYTHONPATH
elif [ -n "$STONE_ROOT" ]; then
    [ -z "$STONEFS_SHELL" ] && STONEFS_SHELL=$STONE_ROOT/src/tools/stonefs/stonefs-shell
    [ -z "$PYBIND" ] && PYBIND=$STONE_ROOT/src/pybind
    [ -z "$STONE_BIN" ] && STONE_BIN=$STONE_BUILD_DIR/bin
    [ -z "$STONE_ADM" ] && STONE_ADM=$STONE_BIN/stone
    [ -z "$INIT_STONE" ] && INIT_STONE=$STONE_BIN/init-stone
    [ -z "$STONE_LIB" ] && STONE_LIB=$STONE_BUILD_DIR/lib
    [ -z "$STONE_EXT_LIB" ] && STONE_EXT_LIB=$STONE_BUILD_DIR/external/lib
    [ -z "$OBJCLASS_PATH" ] && OBJCLASS_PATH=$STONE_LIB
    [ -z "$EC_PATH" ] && EC_PATH=$STONE_LIB
    [ -z "$STONE_PYTHON_COMMON" ] && STONE_PYTHON_COMMON=$STONE_ROOT/src/python-common
fi

if [ -z "${STONE_VSTART_WRAPPER}" ]; then
    PATH=$(pwd):$PATH
fi

[ -z "$PYBIND" ] && PYBIND=./pybind

[ -n "$STONE_PYTHON_COMMON" ] && STONE_PYTHON_COMMON="$STONE_PYTHON_COMMON:"
CYTHON_PYTHONPATH="$STONE_LIB/cython_modules/lib.3"
export PYTHONPATH=$PYBIND:$CYTHON_PYTHONPATH:$STONE_PYTHON_COMMON$PYTHONPATH

export LD_LIBRARY_PATH=$STONE_LIB:$STONE_EXT_LIB:$LD_LIBRARY_PATH
export DYLD_LIBRARY_PATH=$STONE_LIB:$STONE_EXT_LIB:$DYLD_LIBRARY_PATH
# Suppress logging for regular use that indicated that we are using a
# development version. vstart.sh is only used during testing and
# development
export STONE_DEV=1

[ -z "$STONE_NUM_MON" ] && STONE_NUM_MON="$MON"
[ -z "$STONE_NUM_OSD" ] && STONE_NUM_OSD="$OSD"
[ -z "$STONE_NUM_MDS" ] && STONE_NUM_MDS="$MDS"
[ -z "$STONE_NUM_MGR" ] && STONE_NUM_MGR="$MGR"
[ -z "$STONE_NUM_FS"  ] && STONE_NUM_FS="$FS"
[ -z "$STONE_NUM_RGW" ] && STONE_NUM_RGW="$RGW"
[ -z "$GANESHA_DAEMON_NUM" ] && GANESHA_DAEMON_NUM="$NFS"

# if none of the STONE_NUM_* number is specified, kill the existing
# cluster.
if [ -z "$STONE_NUM_MON" -a \
     -z "$STONE_NUM_OSD" -a \
     -z "$STONE_NUM_MDS" -a \
     -z "$STONE_NUM_MGR" -a \
     -z "$GANESHA_DAEMON_NUM" ]; then
    kill_all=1
else
    kill_all=0
fi

[ -z "$STONE_NUM_MON" ] && STONE_NUM_MON=3
[ -z "$STONE_NUM_OSD" ] && STONE_NUM_OSD=3
[ -z "$STONE_NUM_MDS" ] && STONE_NUM_MDS=3
[ -z "$STONE_NUM_MGR" ] && STONE_NUM_MGR=1
[ -z "$STONE_NUM_FS"  ] && STONE_NUM_FS=1
[ -z "$STONE_MAX_MDS" ] && STONE_MAX_MDS=1
[ -z "$STONE_NUM_RGW" ] && STONE_NUM_RGW=0
[ -z "$GANESHA_DAEMON_NUM" ] && GANESHA_DAEMON_NUM=0

[ -z "$STONE_DIR" ] && STONE_DIR="$PWD"
[ -z "$STONE_DEV_DIR" ] && STONE_DEV_DIR="$STONE_DIR/dev"
[ -z "$STONE_OUT_DIR" ] && STONE_OUT_DIR="$STONE_DIR/out"
[ -z "$STONE_RGW_PORT" ] && STONE_RGW_PORT=8000
[ -z "$STONE_CONF_PATH" ] && STONE_CONF_PATH=$STONE_DIR

if [ $STONE_NUM_OSD -gt 3 ]; then
    OSD_POOL_DEFAULT_SIZE=3
else
    OSD_POOL_DEFAULT_SIZE=$STONE_NUM_OSD
fi

extra_conf=""
new=0
standby=0
debug=0
ip=""
nodaemon=0
redirect=0
smallmds=0
short=0
ec=0
stoneadm=0
parallel=true
hitset=""
overwrite_conf=0
stonex=1 #turn stonex on by default
gssapi_authx=0
cache=""
if [ `uname` = FreeBSD ]; then
    objectstore="filestore"
else
    objectstore="bluestore"
fi
stone_osd=stone-osd
rgw_frontend="beast"
rgw_compression=""
lockdep=${LOCKDEP:-1}
spdk_enabled=0 #disable SPDK by default
zoned_enabled=0
io_uring_enabled=0
with_jaeger=0

with_mgr_dashboard=true
if [[ "$(get_cmake_variable WITH_MGR_DASHBOARD_FRONTEND)" != "ON" ]] ||
   [[ "$(get_cmake_variable WITH_RBD)" != "ON" ]]; then
    debug echo "stone-mgr dashboard not built - disabling."
    with_mgr_dashboard=false
fi

filestore_path=
kstore_path=
bluestore_dev=

VSTART_SEC="client.vstart.sh"

MON_ADDR=""
DASH_URLS=""
RESTFUL_URLS=""

conf_fn="$STONE_CONF_PATH/stone.conf"
keyring_fn="$STONE_CONF_PATH/keyring"
osdmap_fn="/tmp/stone_osdmap.$$"
monmap_fn="/tmp/stone_monmap.$$"
inc_osd_num=0

msgr="21"

usage="usage: $0 [option]... \nex: MON=3 OSD=1 MDS=1 MGR=1 RGW=1 NFS=1 $0 -n -d\n"
usage=$usage"options:\n"
usage=$usage"\t-d, --debug\n"
usage=$usage"\t-s, --standby_mds: Generate standby-replay MDS for each active\n"
usage=$usage"\t-l, --localhost: use localhost instead of hostname\n"
usage=$usage"\t-i <ip>: bind to specific ip\n"
usage=$usage"\t-n, --new\n"
usage=$usage"\t--valgrind[_{osd,mds,mon,rgw}] 'toolname args...'\n"
usage=$usage"\t--nodaemon: use stone-run as wrapper for mon/osd/mds\n"
usage=$usage"\t--redirect-output: only useful with nodaemon, directs output to log file\n"
usage=$usage"\t--smallmds: limit mds cache memory limit\n"
usage=$usage"\t-m ip:port\t\tspecify monitor address\n"
usage=$usage"\t-k keep old configuration files (default)\n"
usage=$usage"\t-x enable stonex (on by default)\n"
usage=$usage"\t-X disable stonex\n"
usage=$usage"\t-g --gssapi enable Kerberos/GSSApi authentication\n"
usage=$usage"\t-G disable Kerberos/GSSApi authentication\n"
usage=$usage"\t--hitset <pool> <hit_set_type>: enable hitset tracking\n"
usage=$usage"\t-e : create an erasure pool\n";
usage=$usage"\t-o config\t\t add extra config parameters to all sections\n"
usage=$usage"\t--rgw_port specify stone rgw http listen port\n"
usage=$usage"\t--rgw_frontend specify the rgw frontend configuration\n"
usage=$usage"\t--rgw_compression specify the rgw compression plugin\n"
usage=$usage"\t-b, --bluestore use bluestore as the osd objectstore backend (default)\n"
usage=$usage"\t-f, --filestore use filestore as the osd objectstore backend\n"
usage=$usage"\t-K, --kstore use kstore as the osd objectstore backend\n"
usage=$usage"\t--memstore use memstore as the osd objectstore backend\n"
usage=$usage"\t--cache <pool>: enable cache tiering on pool\n"
usage=$usage"\t--short: short object names only; necessary for ext4 dev\n"
usage=$usage"\t--nolockdep disable lockdep\n"
usage=$usage"\t--multimds <count> allow multimds with maximum active count\n"
usage=$usage"\t--without-dashboard: do not run using mgr dashboard\n"
usage=$usage"\t--bluestore-spdk: enable SPDK and with a comma-delimited list of PCI-IDs of NVME device (e.g, 0000:81:00.0)\n"
usage=$usage"\t--msgr1: use msgr1 only\n"
usage=$usage"\t--msgr2: use msgr2 only\n"
usage=$usage"\t--msgr21: use msgr2 and msgr1\n"
usage=$usage"\t--crimson: use crimson-osd instead of stone-osd\n"
usage=$usage"\t--osd-args: specify any extra osd specific options\n"
usage=$usage"\t--bluestore-devs: comma-separated list of blockdevs to use for bluestore\n"
usage=$usage"\t--bluestore-zoned: blockdevs listed by --bluestore-devs are zoned devices (HM-SMR HDD or ZNS SSD)\n"
usage=$usage"\t--bluestore-io-uring: enable io_uring backend\n"
usage=$usage"\t--inc-osd: append some more osds into existing vcluster\n"
usage=$usage"\t--stoneadm: enable stoneadm orchestrator with ~/.ssh/id_rsa[.pub]\n"
usage=$usage"\t--no-parallel: dont start all OSDs in parallel\n"
usage=$usage"\t--jaeger: use jaegertracing for tracing\n"

usage_exit() {
    printf "$usage"
    exit
}

while [ $# -ge 1 ]; do
case $1 in
    -d | --debug)
        debug=1
        ;;
    -s | --standby_mds)
        standby=1
        ;;
    -l | --localhost)
        ip="127.0.0.1"
        ;;
    -i)
        [ -z "$2" ] && usage_exit
        ip="$2"
        shift
        ;;
    -e)
        ec=1
        ;;
    --new | -n)
        new=1
        ;;
    --inc-osd)
        new=0
        kill_all=0
        inc_osd_num=$2
        if [ "$inc_osd_num" == "" ]; then
            inc_osd_num=1
        else
            shift
        fi
        ;;
    --short)
        short=1
        ;;
    --crimson)
        stone_osd=crimson-osd
        ;;
    --osd-args)
        extra_osd_args="$2"
        shift
        ;;
    --msgr1)
        msgr="1"
        ;;
    --msgr2)
        msgr="2"
        ;;
    --msgr21)
        msgr="21"
        ;;
    --stoneadm)
        stoneadm=1
        ;;
    --no-parallel)
        parallel=false
        ;;
    --valgrind)
        [ -z "$2" ] && usage_exit
        valgrind=$2
        shift
        ;;
    --valgrind_args)
        valgrind_args="$2"
        shift
        ;;
    --valgrind_mds)
        [ -z "$2" ] && usage_exit
        valgrind_mds=$2
        shift
        ;;
    --valgrind_osd)
        [ -z "$2" ] && usage_exit
        valgrind_osd=$2
        shift
        ;;
    --valgrind_mon)
        [ -z "$2" ] && usage_exit
        valgrind_mon=$2
        shift
        ;;
    --valgrind_mgr)
        [ -z "$2" ] && usage_exit
        valgrind_mgr=$2
        shift
        ;;
    --valgrind_rgw)
        [ -z "$2" ] && usage_exit
        valgrind_rgw=$2
        shift
        ;;
    --nodaemon)
        nodaemon=1
        ;;
    --redirect-output)
        redirect=1
        ;;
    --smallmds)
        smallmds=1
        ;;
    --rgw_port)
        STONE_RGW_PORT=$2
        shift
        ;;
    --rgw_frontend)
        rgw_frontend=$2
        shift
        ;;
    --rgw_compression)
        rgw_compression=$2
        shift
        ;;
    --kstore_path)
        kstore_path=$2
        shift
        ;;
    --filestore_path)
        filestore_path=$2
        shift
        ;;
    -m)
        [ -z "$2" ] && usage_exit
        MON_ADDR=$2
        shift
        ;;
    -x)
        stonex=1 # this is on be default, flag exists for historical consistency
        ;;
    -X)
        stonex=0
        ;;

    -g | --gssapi)
        gssapi_authx=1
        ;;
    -G)
        gssapi_authx=0
        ;;

    -k)
        if [ ! -r $conf_fn ]; then
            echo "cannot use old configuration: $conf_fn not readable." >&2
            exit
        fi
        new=0
        ;;
    --memstore)
        objectstore="memstore"
        ;;
    -b | --bluestore)
        objectstore="bluestore"
        ;;
    -f | --filestore)
        objectstore="filestore"
        ;;
    -K | --kstore)
        objectstore="kstore"
        ;;
    --hitset)
        hitset="$hitset $2 $3"
        shift
        shift
        ;;
    -o)
        extra_conf+=$'\n'"$2"
        shift
        ;;
    --cache)
        if [ -z "$cache" ]; then
            cache="$2"
        else
            cache="$cache $2"
        fi
        shift
        ;;
    --nolockdep)
        lockdep=0
        ;;
    --multimds)
        STONE_MAX_MDS="$2"
        shift
        ;;
    --without-dashboard)
        with_mgr_dashboard=false
        ;;
    --bluestore-spdk)
        [ -z "$2" ] && usage_exit
        IFS=',' read -r -a bluestore_spdk_dev <<< "$2"
        spdk_enabled=1
        shift
        ;;
    --bluestore-devs)
        IFS=',' read -r -a bluestore_dev <<< "$2"
        for dev in "${bluestore_dev[@]}"; do
            if [ ! -b $dev -o ! -w $dev ]; then
                echo "All --bluestore-devs must refer to writable block devices"
                exit 1
            fi
        done
        shift
        ;;
    --bluestore-zoned)
        zoned_enabled=1
        ;;
    --bluestore-io-uring)
        io_uring_enabled=1
        shift
        ;;
    --jaeger)
        with_jaeger=1
        echo "with_jaeger $with_jaeger"
        ;;
    *)
        usage_exit
esac
shift
done

if [ $kill_all -eq 1 ]; then
    $SUDO $INIT_STONE stop
fi

if [ "$new" -eq 0 ]; then
    if [ -z "$STONE_ASOK_DIR" ]; then
        STONE_ASOK_DIR=`dirname $($STONE_BIN/stone-conf  -c $conf_fn --show-config-value admin_socket)`
    fi
    mkdir -p $STONE_ASOK_DIR
    MON=`$STONE_BIN/stone-conf -c $conf_fn --name $VSTART_SEC --lookup num_mon 2>/dev/null` && \
        STONE_NUM_MON="$MON"
    OSD=`$STONE_BIN/stone-conf -c $conf_fn --name $VSTART_SEC --lookup num_osd 2>/dev/null` && \
        STONE_NUM_OSD="$OSD"
    MDS=`$STONE_BIN/stone-conf -c $conf_fn --name $VSTART_SEC --lookup num_mds 2>/dev/null` && \
        STONE_NUM_MDS="$MDS"
    MGR=`$STONE_BIN/stone-conf -c $conf_fn --name $VSTART_SEC --lookup num_mgr 2>/dev/null` && \
        STONE_NUM_MGR="$MGR"
    RGW=`$STONE_BIN/stone-conf -c $conf_fn --name $VSTART_SEC --lookup num_rgw 2>/dev/null` && \
        STONE_NUM_RGW="$RGW"
    NFS=`$STONE_BIN/stone-conf -c $conf_fn --name $VSTART_SEC --lookup num_ganesha 2>/dev/null` && \
        GANESHA_DAEMON_NUM="$NFS"
else
    # only delete if -n
    if [ -e "$conf_fn" ]; then
        asok_dir=`dirname $($STONE_BIN/stone-conf  -c $conf_fn --show-config-value admin_socket)`
        rm -- "$conf_fn"
        if [ $asok_dir != /var/run/stone ]; then
            [ -d $asok_dir ] && rm -f $asok_dir/* && rmdir $asok_dir
        fi
    fi
    if [ -z "$STONE_ASOK_DIR" ]; then
        STONE_ASOK_DIR=`mktemp -u -d "${TMPDIR:-/tmp}/stone-asok.XXXXXX"`
    fi
fi

ARGS="-c $conf_fn"

run() {
    type=$1
    shift
    num=$1
    shift
    eval "valg=\$valgrind_$type"
    [ -z "$valg" ] && valg="$valgrind"

    if [ -n "$valg" ]; then
        prunb valgrind --tool="$valg" $valgrind_args "$@" -f
        sleep 1
    else
        if [ "$nodaemon" -eq 0 ]; then
            prun "$@"
        elif [ "$redirect" -eq 0 ]; then
            prunb ${STONE_ROOT}/src/stone-run "$@" -f
        else
            ( prunb ${STONE_ROOT}/src/stone-run "$@" -f ) >$STONE_OUT_DIR/$type.$num.stdout 2>&1
        fi
    fi
}

wconf() {
    if [ "$new" -eq 1 -o "$overwrite_conf" -eq 1 ]; then
        cat >> "$conf_fn"
    fi
}


do_rgw_conf() {

    if [ $STONE_NUM_RGW -eq 0 ]; then
        return 0
    fi

    # setup each rgw on a sequential port, starting at $STONE_RGW_PORT.
    # individual rgw's ids will be their ports.
    current_port=$STONE_RGW_PORT
    for n in $(seq 1 $STONE_NUM_RGW); do
        wconf << EOF
[client.rgw.${current_port}]
        rgw frontends = $rgw_frontend port=${current_port}
        admin socket = ${STONE_OUT_DIR}/radosgw.${current_port}.asok
EOF
        current_port=$((current_port + 1))
done

}

format_conf() {
    local opts=$1
    local indent="        "
    local opt
    local formatted
    while read -r opt; do
        if [ -z "$formatted" ]; then
            formatted="${opt}"
        else
            formatted+=$'\n'${indent}${opt}
        fi
    done <<< "$opts"
    echo "$formatted"
}

prepare_conf() {
    local DAEMONOPTS="
        log file = $STONE_OUT_DIR/\$name.log
        admin socket = $STONE_ASOK_DIR/\$name.asok
        chdir = \"\"
        pid file = $STONE_OUT_DIR/\$name.pid
        heartbeat file = $STONE_OUT_DIR/\$name.heartbeat
"

    local mgr_modules="restful iostat nfs"
    if $with_mgr_dashboard; then
        mgr_modules="dashboard $mgr_modules"
    fi

    local msgr_conf=''
    if [ $msgr -eq 21 ]; then
        msgr_conf="ms bind msgr2 = true
                   ms bind msgr1 = true"
    fi
    if [ $msgr -eq 2 ]; then
        msgr_conf="ms bind msgr2 = true
                   ms bind msgr1 = false"
    fi
    if [ $msgr -eq 1 ]; then
        msgr_conf="ms bind msgr2 = false
                   ms bind msgr1 = true"
    fi

    wconf <<EOF
; generated by vstart.sh on `date`
[$VSTART_SEC]
        num mon = $STONE_NUM_MON
        num osd = $STONE_NUM_OSD
        num mds = $STONE_NUM_MDS
        num mgr = $STONE_NUM_MGR
        num rgw = $STONE_NUM_RGW
        num ganesha = $GANESHA_DAEMON_NUM

[global]
        fsid = $(uuidgen)
        osd failsafe full ratio = .99
        mon osd full ratio = .99
        mon osd nearfull ratio = .99
        mon osd backfillfull ratio = .99
        mon_max_pg_per_osd = ${MON_MAX_PG_PER_OSD:-1000}
        erasure code dir = $EC_PATH
        plugin dir = $STONE_LIB
        filestore fd cache size = 32
        run dir = $STONE_OUT_DIR
        crash dir = $STONE_OUT_DIR
        enable experimental unrecoverable data corrupting features = *
        osd_crush_chooseleaf_type = 0
        debug asok assert abort = true
        $(format_conf "${msgr_conf}")
        $(format_conf "${extra_conf}")
EOF
    if [ "$lockdep" -eq 1 ] ; then
        wconf <<EOF
        lockdep = true
EOF
    fi
    if [ "$stonex" -eq 1 ] ; then
        wconf <<EOF
        auth cluster required = stonex
        auth service required = stonex
        auth client required = stonex
EOF
    elif [ "$gssapi_authx" -eq 1 ] ; then
        wconf <<EOF
        auth cluster required = gss
        auth service required = gss
        auth client required = gss
        gss ktab client file = $STONE_DEV_DIR/gss_\$name.keytab
EOF
    else
        wconf <<EOF
        auth cluster required = none
        auth service required = none
        auth client required = none
EOF
    fi
    if [ "$short" -eq 1 ]; then
        COSDSHORT="        osd max object name len = 460
        osd max object namespace len = 64"
    fi
    if [ "$objectstore" == "bluestore" ]; then
        if [ "$spdk_enabled" -eq 1 ]; then
            BLUESTORE_OPTS="        bluestore_block_db_path = \"\"
        bluestore_block_db_size = 0
        bluestore_block_db_create = false
        bluestore_block_wal_path = \"\"
        bluestore_block_wal_size = 0
        bluestore_block_wal_create = false
        bluestore_spdk_mem = 2048"
        else
            BLUESTORE_OPTS="        bluestore block db path = $STONE_DEV_DIR/osd\$id/block.db.file
        bluestore block db size = 1073741824
        bluestore block db create = true
        bluestore block wal path = $STONE_DEV_DIR/osd\$id/block.wal.file
        bluestore block wal size = 1048576000
        bluestore block wal create = true"
        fi
        if [ "$zoned_enabled" -eq 1 ]; then
            BLUESTORE_OPTS+="
        bluestore min alloc size = 65536
        bluestore prefer deferred size = 0
        bluestore prefer deferred size hdd = 0
        bluestore prefer deferred size ssd = 0
        bluestore allocator = zoned"
        fi
        if [ "$io_uring_enabled" -eq 1 ]; then
            BLUESTORE_OPTS+="
        bdev ioring = true"
        fi
    fi
    wconf <<EOF
[client]
        keyring = $keyring_fn
        log file = $STONE_OUT_DIR/\$name.\$pid.log
        admin socket = $STONE_ASOK_DIR/\$name.\$pid.asok

        ; needed for s3tests
        rgw crypt s3 kms backend = testing
        rgw crypt s3 kms encryption keys = testkey-1=YmluCmJvb3N0CmJvb3N0LWJ1aWxkCmNlcGguY29uZgo= testkey-2=aWIKTWFrZWZpbGUKbWFuCm91dApzcmMKVGVzdGluZwo=
        rgw crypt require ssl = false
        ; uncomment the following to set LC days as the value in seconds;
        ; needed for passing lc time based s3-tests (can be verbose)
        ; rgw lc debug interval = 10
        $(format_conf "${extra_conf}")
EOF
	do_rgw_conf
	wconf << EOF
[mds]
$DAEMONOPTS
        mds data = $STONE_DEV_DIR/mds.\$id
        mds root ino uid = `id -u`
        mds root ino gid = `id -g`
        $(format_conf "${extra_conf}")
[mgr]
        mgr data = $STONE_DEV_DIR/mgr.\$id
        mgr module path = $MGR_PYTHON_PATH
        stoneadm path = $STONE_ROOT/src/stoneadm/stoneadm
$DAEMONOPTS
        $(format_conf "${extra_conf}")
[osd]
$DAEMONOPTS
        osd_check_max_object_name_len_on_startup = false
        osd data = $STONE_DEV_DIR/osd\$id
        osd journal = $STONE_DEV_DIR/osd\$id/journal
        osd journal size = 100
        osd class tmp = out
        osd class dir = $OBJCLASS_PATH
        osd class load list = *
        osd class default list = *
        osd fast shutdown = false

        filestore wbthrottle xfs ios start flusher = 10
        filestore wbthrottle xfs ios hard limit = 20
        filestore wbthrottle xfs inodes hard limit = 30
        filestore wbthrottle btrfs ios start flusher = 10
        filestore wbthrottle btrfs ios hard limit = 20
        filestore wbthrottle btrfs inodes hard limit = 30
        bluestore fsck on mount = true
        bluestore block create = true
$BLUESTORE_OPTS

        ; kstore
        kstore fsck on mount = true
        osd objectstore = $objectstore
$COSDSHORT
        $(format_conf "${extra_conf}")
[mon]
        mgr initial modules = $mgr_modules
$DAEMONOPTS
$CMONDEBUG
        $(format_conf "${extra_conf}")
        mon cluster log file = $STONE_OUT_DIR/cluster.mon.\$id.log
        osd pool default erasure code profile = plugin=jerasure technique=reed_sol_van k=2 m=1 crush-failure-domain=osd
        auth allow insecure global id reclaim = false
EOF
}

write_logrotate_conf() {
    out_dir=$(pwd)"/out/*.log"

    cat << EOF
$out_dir
{
    rotate 5
    size 1G
    copytruncate
    compress
    notifempty
    missingok
    sharedscripts
    postrotate
        # NOTE: assuring that the absence of one of the following processes
        # won't abort the logrotate command.
        killall -u $USER -q -1 stone-mon stone-mgr stone-mds stone-osd stone-fuse radosgw rbd-mirror || echo ""
    endscript
}
EOF
}

init_logrotate() {
    logrotate_conf_path=$(pwd)"/logrotate.conf"
    logrotate_state_path=$(pwd)"/logrotate.state"

    if ! test -a $logrotate_conf_path; then
        if test -a $logrotate_state_path; then
            rm -f $logrotate_state_path
        fi
        write_logrotate_conf > $logrotate_conf_path
    fi
}

start_mon() {
    local MONS=""
    local count=0
    for f in a b c d e f g h i j k l m n o p q r s t u v w x y z
    do
        [ $count -eq $STONE_NUM_MON ] && break;
        count=$(($count + 1))
        if [ -z "$MONS" ]; then
	    MONS="$f"
        else
	    MONS="$MONS $f"
        fi
    done

    if [ "$new" -eq 1 ]; then
        if [ `echo $IP | grep '^127\\.'` ]; then
            echo
            echo "NOTE: hostname resolves to loopback; remote hosts will not be able to"
            echo "  connect.  either adjust /etc/hosts, or edit this script to use your"
            echo "  machine's real IP."
            echo
        fi

        prun $SUDO "$STONE_BIN/stone-authtool" --create-keyring --gen-key --name=mon. "$keyring_fn" --cap mon 'allow *'
        prun $SUDO "$STONE_BIN/stone-authtool" --gen-key --name=client.admin \
             --cap mon 'allow *' \
             --cap osd 'allow *' \
             --cap mds 'allow *' \
             --cap mgr 'allow *' \
             "$keyring_fn"

        # build a fresh fs monmap, mon fs
        local params=()
        local count=0
        local mon_host=""
        for f in $MONS
        do
            if [ $msgr -eq 1 ]; then
                A="v1:$IP:$(($STONE_PORT+$count+1))"
            fi
            if [ $msgr -eq 2 ]; then
                A="v2:$IP:$(($STONE_PORT+$count+1))"
            fi
            if [ $msgr -eq 21 ]; then
                A="[v2:$IP:$(($STONE_PORT+$count)),v1:$IP:$(($STONE_PORT+$count+1))]"
            fi
            params+=("--addv" "$f" "$A")
            mon_host="$mon_host $A"
            wconf <<EOF
[mon.$f]
        host = $HOSTNAME
        mon data = $STONE_DEV_DIR/mon.$f
EOF
            count=$(($count + 2))
        done
        wconf <<EOF
[global]
        mon host = $mon_host
EOF
        prun "$STONE_BIN/monmaptool" --create --clobber "${params[@]}" --print "$monmap_fn"

        for f in $MONS
        do
            prun rm -rf -- "$STONE_DEV_DIR/mon.$f"
            prun mkdir -p "$STONE_DEV_DIR/mon.$f"
            prun "$STONE_BIN/stone-mon" --mkfs -c "$conf_fn" -i "$f" --monmap="$monmap_fn" --keyring="$keyring_fn"
        done

        prun rm -- "$monmap_fn"
    fi

    # start monitors
    for f in $MONS
    do
        run 'mon' $f $STONE_BIN/stone-mon -i $f $ARGS $CMON_ARGS
    done
}

start_osd() {
    if [ $inc_osd_num -gt 0 ]; then
        old_maxosd=$($STONE_BIN/stone osd getmaxosd | sed -e 's/max_osd = //' -e 's/ in epoch.*//')
        start=$old_maxosd
        end=$(($start-1+$inc_osd_num))
        overwrite_conf=1 # fake wconf
    else
        start=0
        end=$(($STONE_NUM_OSD-1))
    fi
    local osds_wait
    for osd in `seq $start $end`
    do
	local extra_seastar_args
	if [ "$stone_osd" == "crimson-osd" ]; then
	    # designate a single CPU node $osd for osd.$osd
	    extra_seastar_args="--smp 1 --cpuset $osd"
	    if [ "$debug" -ne 0 ]; then
		extra_seastar_args+=" --debug"
	    fi
	fi
	if [ "$new" -eq 1 -o $inc_osd_num -gt 0 ]; then
            wconf <<EOF
[osd.$osd]
        host = $HOSTNAME
EOF
            if [ "$spdk_enabled" -eq 1 ]; then
                wconf <<EOF
        bluestore_block_path = spdk:${bluestore_spdk_dev[$osd]}
EOF
            fi

            rm -rf $STONE_DEV_DIR/osd$osd || true
            if command -v btrfs > /dev/null; then
                for f in $STONE_DEV_DIR/osd$osd/*; do btrfs sub delete $f &> /dev/null || true; done
            fi
            if [ -n "$filestore_path" ]; then
                ln -s $filestore_path $STONE_DEV_DIR/osd$osd
            elif [ -n "$kstore_path" ]; then
                ln -s $kstore_path $STONE_DEV_DIR/osd$osd
            else
                mkdir -p $STONE_DEV_DIR/osd$osd
                if [ -n "${bluestore_dev[$osd]}" ]; then
                    dd if=/dev/zero of=${bluestore_dev[$osd]} bs=1M count=1
                    ln -s ${bluestore_dev[$osd]} $STONE_DEV_DIR/osd$osd/block
                    wconf <<EOF
        bluestore fsck on mount = false
EOF
                fi
            fi

            local uuid=`uuidgen`
            echo "add osd$osd $uuid"
            OSD_SECRET=$($STONE_BIN/stone-authtool --gen-print-key)
            echo "{\"stonex_secret\": \"$OSD_SECRET\"}" > $STONE_DEV_DIR/osd$osd/new.json
            stone_adm osd new $uuid -i $STONE_DEV_DIR/osd$osd/new.json
            rm $STONE_DEV_DIR/osd$osd/new.json
            $SUDO $STONE_BIN/$stone_osd $extra_osd_args -i $osd $ARGS --mkfs --key $OSD_SECRET --osd-uuid $uuid $extra_seastar_args

            local key_fn=$STONE_DEV_DIR/osd$osd/keyring
            cat > $key_fn<<EOF
[osd.$osd]
        key = $OSD_SECRET
EOF
        fi
        echo start osd.$osd
        local osd_pid
        run 'osd' $osd $SUDO $STONE_BIN/$stone_osd \
            $extra_seastar_args $extra_osd_args \
            -i $osd $ARGS $COSD_ARGS &
        osd_pid=$!
        if $parallel; then
            osds_wait=$osd_pid
        else
            wait $osd_pid
        fi
    done
    if $parallel; then
        for p in $osds_wait; do
            wait $p
        done
        debug echo OSDs started
    fi
    if [ $inc_osd_num -gt 0 ]; then
        # update num osd
        new_maxosd=$($STONE_BIN/stone osd getmaxosd | sed -e 's/max_osd = //' -e 's/ in epoch.*//')
        sed -i "s/num osd = .*/num osd = $new_maxosd/g" $conf_fn
    fi
}

start_mgr() {
    local mgr=0
    local ssl=${DASHBOARD_SSL:-1}
    # avoid monitors on nearby ports (which test/*.sh use extensively)
    MGR_PORT=$(($STONE_PORT + 1000))
    PROMETHEUS_PORT=9283
    for name in x y z a b c d e f g h i j k l m n o p
    do
        [ $mgr -eq $STONE_NUM_MGR ] && break
        mgr=$(($mgr + 1))
        if [ "$new" -eq 1 ]; then
            mkdir -p $STONE_DEV_DIR/mgr.$name
            key_fn=$STONE_DEV_DIR/mgr.$name/keyring
            $SUDO $STONE_BIN/stone-authtool --create-keyring --gen-key --name=mgr.$name $key_fn
            stone_adm -i $key_fn auth add mgr.$name mon 'allow profile mgr' mds 'allow *' osd 'allow *'

            wconf <<EOF
[mgr.$name]
        host = $HOSTNAME
EOF

            if $with_mgr_dashboard ; then
                local port_option="ssl_server_port"
                local http_proto="https"
                if [ "$ssl" == "0" ]; then
                    port_option="server_port"
                    http_proto="http"
                    stone_adm config set mgr mgr/dashboard/ssl false --force
                fi
                stone_adm config set mgr mgr/dashboard/$name/$port_option $MGR_PORT --force
                if [ $mgr -eq 1 ]; then
                    DASH_URLS="$http_proto://$IP:$MGR_PORT"
                else
                    DASH_URLS+=", $http_proto://$IP:$MGR_PORT"
                fi
            fi
	    MGR_PORT=$(($MGR_PORT + 1000))
	    stone_adm config set mgr mgr/prometheus/$name/server_port $PROMETHEUS_PORT --force
	    PROMETHEUS_PORT=$(($PROMETHEUS_PORT + 1000))

	    stone_adm config set mgr mgr/restful/$name/server_port $MGR_PORT --force
            if [ $mgr -eq 1 ]; then
                RESTFUL_URLS="https://$IP:$MGR_PORT"
            else
                RESTFUL_URLS+=", https://$IP:$MGR_PORT"
            fi
	    MGR_PORT=$(($MGR_PORT + 1000))
        fi

        debug echo "Starting mgr.${name}"
        run 'mgr' $name $STONE_BIN/stone-mgr -i $name $ARGS
    done

    if [ "$new" -eq 1 ]; then
        # setting login credentials for dashboard
        if $with_mgr_dashboard; then
            while ! stone_adm -h | grep -c -q ^dashboard ; do
                debug echo 'waiting for mgr dashboard module to start'
                sleep 1
            done
            DASHBOARD_ADMIN_SECRET_FILE="${STONE_CONF_PATH}/dashboard-admin-secret.txt"
            printf 'admin' > "${DASHBOARD_ADMIN_SECRET_FILE}"
            stone_adm dashboard ac-user-create admin -i "${DASHBOARD_ADMIN_SECRET_FILE}" \
                administrator --force-password
            if [ "$ssl" != "0" ]; then
                if ! stone_adm dashboard create-self-signed-cert;  then
                    debug echo dashboard module not working correctly!
                fi
            fi
        fi

        while ! stone_adm -h | grep -c -q ^restful ; do
            debug echo 'waiting for mgr restful module to start'
            sleep 1
        done
        if stone_adm restful create-self-signed-cert; then
            SF=`mktemp`
            stone_adm restful create-key admin -o $SF
            RESTFUL_SECRET=`cat $SF`
            rm $SF
        else
            debug echo MGR Restful is not working, perhaps the package is not installed?
        fi
    fi

    if [ "$stoneadm" -eq 1 ]; then
        debug echo Enabling stoneadm orchestrator
	if [ "$new" -eq 1 ]; then
		digest=$(curl -s \
		https://registry.hub.docker.com/v2/repositories/stone/daemon-base/tags/latest-master-devel \
		| jq -r '.images[0].digest')
		stone_adm config set global container_image "docker.io/stone/daemon-base@$digest"
	fi
        stone_adm config-key set mgr/stoneadm/ssh_identity_key -i ~/.ssh/id_rsa
        stone_adm config-key set mgr/stoneadm/ssh_identity_pub -i ~/.ssh/id_rsa.pub
        stone_adm mgr module enable stoneadm
        stone_adm orch set backend stoneadm
        stone_adm orch host add "$(hostname)"
        stone_adm orch apply crash '*'
        stone_adm config set mgr mgr/stoneadm/allow_ptrace true
    fi
}

start_mds() {
    local mds=0
    for name in a b c d e f g h i j k l m n o p
    do
        [ $mds -eq $STONE_NUM_MDS ] && break
        mds=$(($mds + 1))

        if [ "$new" -eq 1 ]; then
            prun mkdir -p "$STONE_DEV_DIR/mds.$name"
            key_fn=$STONE_DEV_DIR/mds.$name/keyring
            wconf <<EOF
[mds.$name]
        host = $HOSTNAME
EOF
            if [ "$standby" -eq 1 ]; then
                mkdir -p $STONE_DEV_DIR/mds.${name}s
                wconf <<EOF
        mds standby for rank = $mds
[mds.${name}s]
        mds standby replay = true
        mds standby for name = ${name}
EOF
            fi
            prun $SUDO "$STONE_BIN/stone-authtool" --create-keyring --gen-key --name="mds.$name" "$key_fn"
            stone_adm -i "$key_fn" auth add "mds.$name" mon 'allow profile mds' osd 'allow rw tag stonefs *=*' mds 'allow' mgr 'allow profile mds'
            if [ "$standby" -eq 1 ]; then
                prun $SUDO "$STONE_BIN/stone-authtool" --create-keyring --gen-key --name="mds.${name}s" \
                     "$STONE_DEV_DIR/mds.${name}s/keyring"
                stone_adm -i "$STONE_DEV_DIR/mds.${name}s/keyring" auth add "mds.${name}s" \
                             mon 'allow profile mds' osd 'allow *' mds 'allow' mgr 'allow profile mds'
            fi
        fi

        run 'mds' $name $STONE_BIN/stone-mds -i $name $ARGS $CMDS_ARGS
        if [ "$standby" -eq 1 ]; then
            run 'mds' $name $STONE_BIN/stone-mds -i ${name}s $ARGS $CMDS_ARGS
        fi

        #valgrind --tool=massif $STONE_BIN/stone-mds $ARGS --mds_log_max_segments 2 --mds_thrash_fragments 0 --mds_thrash_exports 0 > m  #--debug_ms 20
        #$STONE_BIN/stone-mds -d $ARGS --mds_thrash_fragments 0 --mds_thrash_exports 0 #--debug_ms 20
        #stone_adm mds set max_mds 2
    done

    if [ $new -eq 1 ]; then
        if [ "$STONE_NUM_FS" -gt "0" ] ; then
            sleep 5 # time for MDS to come up as standby to avoid health warnings on fs creation
            if [ "$STONE_NUM_FS" -gt "1" ] ; then
                stone_adm fs flag set enable_multiple true --yes-i-really-mean-it
            fi

	    # wait for volume module to load
	    while ! stone_adm fs volume ls ; do sleep 1 ; done
            local fs=0
            for name in a b c d e f g h i j k l m n o p
            do
                stone_adm fs volume create ${name}
                stone_adm fs authorize ${name} "client.fs_${name}" / rwp >> "$keyring_fn"
                fs=$(($fs + 1))
                [ $fs -eq $STONE_NUM_FS ] && break
            done
        fi
    fi

}

# Ganesha Daemons requires nfs-ganesha nfs-ganesha-stone nfs-ganesha-rados-grace
# nfs-ganesha-rados-urls (version 3.3 and above) packages installed. On
# Fedora>=31 these packages can be installed directly with 'dnf'. For CentOS>=8
# the packages are available at
# https://wiki.centos.org/SpecialInterestGroup/Storage
# Similarly for Ubuntu>=16.04 follow the instructions on
# https://launchpad.net/~nfs-ganesha

start_ganesha() {
    cluster_id="vstart"
    GANESHA_PORT=$(($STONE_PORT + 4000))
    local ganesha=0
    test_user="$cluster_id"
    pool_name=".nfs"
    namespace=$cluster_id
    url="rados://$pool_name/$namespace/conf-nfs.$test_user"

    prun stone_adm auth get-or-create client.$test_user \
        mon "allow r" \
        osd "allow rw pool=$pool_name namespace=$namespace, allow rw tag stonefs data=a" \
        mds "allow rw path=/" \
        >> "$keyring_fn"

    stone_adm mgr module enable test_orchestrator
    stone_adm orch set backend test_orchestrator
    stone_adm test_orchestrator load_data -i $STONE_ROOT/src/pybind/mgr/test_orchestrator/dummy_data.json
    prun stone_adm nfs cluster create $cluster_id
    prun stone_adm nfs export create stonefs --fsname "a" --cluster-id $cluster_id --pseudo-path "/stonefs"

    for name in a b c d e f g h i j k l m n o p
    do
        [ $ganesha -eq $GANESHA_DAEMON_NUM ] && break

        port=$(($GANESHA_PORT + ganesha))
        ganesha=$(($ganesha + 1))
        ganesha_dir="$STONE_DEV_DIR/ganesha.$name"
        prun rm -rf $ganesha_dir
        prun mkdir -p $ganesha_dir

        echo "NFS_CORE_PARAM {
            Enable_NLM = false;
            Enable_RQUOTA = false;
            Protocols = 4;
            NFS_Port = $port;
        }

        MDCACHE {
           Dir_Chunk = 0;
        }

        NFSv4 {
           RecoveryBackend = rados_cluster;
           Minor_Versions = 1, 2;
        }

        RADOS_KV {
           pool = '$pool_name';
           namespace = $namespace;
           UserId = $test_user;
           nodeid = $name;
        }

        RADOS_URLS {
	   Userid = $test_user;
	   watch_url = '$url';
        }

	%url $url" > "$ganesha_dir/ganesha-$name.conf"
	wconf <<EOF
[ganesha.$name]
        host = $HOSTNAME
        ip = $IP
        port = $port
        ganesha data = $ganesha_dir
        pid file = $STONE_OUT_DIR/ganesha-$name.pid
EOF

        prun env STONE_CONF="${conf_fn}" ganesha-rados-grace --userid $test_user -p $pool_name -n $namespace add $name
        prun env STONE_CONF="${conf_fn}" ganesha-rados-grace --userid $test_user -p $pool_name -n $namespace

        prun env STONE_CONF="${conf_fn}" ganesha.nfsd -L "$STONE_OUT_DIR/ganesha-$name.log" -f "$ganesha_dir/ganesha-$name.conf" -p "$STONE_OUT_DIR/ganesha-$name.pid" -N NIV_DEBUG

        # Wait few seconds for grace period to be removed
        sleep 2

        prun env STONE_CONF="${conf_fn}" ganesha-rados-grace --userid $test_user -p $pool_name -n $namespace

        echo "$test_user ganesha daemon $name started on port: $port"
    done
}

if [ "$debug" -eq 0 ]; then
    CMONDEBUG='
        debug mon = 10
        debug ms = 1'
else
    debug echo "** going verbose **"
    CMONDEBUG='
        debug mon = 20
        debug paxos = 20
        debug auth = 20
        debug mgrc = 20
        debug ms = 1'
fi

if [ -n "$MON_ADDR" ]; then
    CMON_ARGS=" -m "$MON_ADDR
    COSD_ARGS=" -m "$MON_ADDR
    CMDS_ARGS=" -m "$MON_ADDR
fi

if [ -z "$STONE_PORT" ]; then
    while [ true ]
    do
        STONE_PORT="$(echo $(( RANDOM % 1000 + 40000 )))"
        ss -a -n | egrep "\<LISTEN\>.+:${STONE_PORT}\s+" 1>/dev/null 2>&1 || break
    done
fi

[ -z "$INIT_STONE" ] && INIT_STONE=$STONE_BIN/init-stone

# sudo if btrfs
[ -d $STONE_DEV_DIR/osd0/. ] && [ -e $STONE_DEV_DIR/sudo ] && SUDO="sudo"

if [ $inc_osd_num -eq 0 ]; then
    prun $SUDO rm -f core*
fi

[ -d $STONE_ASOK_DIR ] || mkdir -p $STONE_ASOK_DIR
[ -d $STONE_OUT_DIR  ] || mkdir -p $STONE_OUT_DIR
[ -d $STONE_DEV_DIR  ] || mkdir -p $STONE_DEV_DIR
if [ $inc_osd_num -eq 0 ]; then
    $SUDO find "$STONE_OUT_DIR" -type f -delete
fi
[ -d gmon ] && $SUDO rm -rf gmon/*

[ "$stonex" -eq 1 ] && [ "$new" -eq 1 ] && [ -e $keyring_fn ] && rm $keyring_fn


# figure machine's ip
HOSTNAME=`hostname -s`
if [ -n "$ip" ]; then
    IP="$ip"
else
    echo hostname $HOSTNAME
    if [ -x "$(which ip 2>/dev/null)" ]; then
        IP_CMD="ip addr"
    else
        IP_CMD="ifconfig"
    fi
    # filter out IPv4 and localhost addresses
    IP="$($IP_CMD | sed -En 's/127.0.0.1//;s/.*inet (addr:)?(([0-9]*\.){3}[0-9]*).*/\2/p' | head -n1)"
    # if nothing left, try using localhost address, it might work
    if [ -z "$IP" ]; then IP="127.0.0.1"; fi
fi
echo "ip $IP"
echo "port $STONE_PORT"


[ -z $STONE_ADM ] && STONE_ADM=$STONE_BIN/stone

stone_adm() {
    if [ "$stonex" -eq 1 ]; then
        prun $SUDO "$STONE_ADM" -c "$conf_fn" -k "$keyring_fn" "$@"
    else
        prun $SUDO "$STONE_ADM" -c "$conf_fn" "$@"
    fi
}

if [ $inc_osd_num -gt 0 ]; then
    start_osd
    exit
fi

if [ "$new" -eq 1 ]; then
    prepare_conf
fi

if [ $STONE_NUM_MON -gt 0 ]; then
    start_mon

    debug echo Populating config ...
    cat <<EOF | $STONE_BIN/stone -c $conf_fn config assimilate-conf -i -
[global]
osd_pool_default_size = $OSD_POOL_DEFAULT_SIZE
osd_pool_default_min_size = 1

[mon]
mon_osd_reporter_subtree_level = osd
mon_data_avail_warn = 2
mon_data_avail_crit = 1
mon_allow_pool_delete = true
mon_allow_pool_size_one = true

[osd]
osd_scrub_load_threshold = 2000
osd_debug_op_order = true
osd_debug_misdirected_ops = true
osd_copyfrom_max_chunk = 524288

[mds]
mds_debug_frag = true
mds_debug_auth_pins = true
mds_debug_subtrees = true

[mgr]
mgr/telemetry/nag = false
mgr/telemetry/enable = false

EOF

    if [ "$debug" -ne 0 ]; then
        debug echo Setting debug configs ...
        cat <<EOF | $STONE_BIN/stone -c $conf_fn config assimilate-conf -i -
[mgr]
debug_ms = 1
debug_mgr = 20
debug_monc = 20
debug_mon = 20

[osd]
debug_ms = 1
debug_osd = 25
debug_objecter = 20
debug_monc = 20
debug_mgrc = 20
debug_journal = 20
debug_filestore = 20
debug_bluestore = 20
debug_bluefs = 20
debug_rocksdb = 20
debug_bdev = 20
debug_reserver = 10
debug_objclass = 20

[mds]
debug_ms = 1
debug_mds = 20
debug_monc = 20
debug_mgrc = 20
mds_debug_scatterstat = true
mds_verify_scatter = true
EOF
    fi
    if [ "$stoneadm" -gt 0 ]; then
        debug echo Setting mon public_network ...
        public_network=$(ip route list | grep -w "$IP" | awk '{print $1}')
        stone_adm config set mon public_network $public_network
    fi
fi

if [ $STONE_NUM_MGR -gt 0 ]; then
    start_mgr
fi

# osd
if [ $STONE_NUM_OSD -gt 0 ]; then
    start_osd
fi

# mds
if [ "$smallmds" -eq 1 ]; then
    wconf <<EOF
[mds]
        mds log max segments = 2
        # Default 'mds cache memory limit' is 1GiB, and here we set it to 100MiB.
        mds cache memory limit = 100M
EOF
fi

if [ $STONE_NUM_MDS -gt 0 ]; then
    start_mds
    # key with access to all FS
    stone_adm fs authorize \* "client.fs" / rwp >> "$keyring_fn"
fi

# Don't set max_mds until all the daemons are started, otherwise
# the intended standbys might end up in active roles.
if [ "$STONE_MAX_MDS" -gt 1 ]; then
    sleep 5  # wait for daemons to make it into FSMap before increasing max_mds
fi
fs=0
for name in a b c d e f g h i j k l m n o p
do
    [ $fs -eq $STONE_NUM_FS ] && break
    fs=$(($fs + 1))
    if [ "$STONE_MAX_MDS" -gt 1 ]; then
        stone_adm fs set "${name}" max_mds "$STONE_MAX_MDS"
    fi
done

# mgr

if [ "$ec" -eq 1 ]; then
    stone_adm <<EOF
osd erasure-code-profile set ec-profile m=2 k=2
osd pool create ec erasure ec-profile
EOF
fi

do_cache() {
    while [ -n "$*" ]; do
        p="$1"
        shift
        debug echo "creating cache for pool $p ..."
        stone_adm <<EOF
osd pool create ${p}-cache
osd tier add $p ${p}-cache
osd tier cache-mode ${p}-cache writeback
osd tier set-overlay $p ${p}-cache
EOF
    done
}
do_cache $cache

do_hitsets() {
    while [ -n "$*" ]; do
        pool="$1"
        type="$2"
        shift
        shift
        debug echo "setting hit_set on pool $pool type $type ..."
        stone_adm <<EOF
osd pool set $pool hit_set_type $type
osd pool set $pool hit_set_count 8
osd pool set $pool hit_set_period 30
EOF
    done
}
do_hitsets $hitset

do_rgw_create_bucket()
{
   # Create RGW Bucket
   local rgw_python_file='rgw-create-bucket.py'
   echo "import boto
import boto.s3.connection

conn = boto.connect_s3(
        aws_access_key_id = '$s3_akey',
        aws_secret_access_key = '$s3_skey',
        host = '$HOSTNAME',
        port = 80,
        is_secure=False,
        calling_format = boto.s3.connection.OrdinaryCallingFormat(),
        )

bucket = conn.create_bucket('nfs-bucket')
print('created new bucket')" > "$STONE_OUT_DIR/$rgw_python_file"
   prun python $STONE_OUT_DIR/$rgw_python_file
}

do_rgw_create_users()
{
    # Create S3 user
    s3_akey='0555b35654ad1656d804'
    s3_skey='h7GhxuBLTrlhVUyxSPUKUV8r/2EI4ngqJxD7iBdBYLhwluN30JaT3Q=='
    debug echo "setting up user testid"
    $STONE_BIN/radosgw-admin user create --uid testid --access-key $s3_akey --secret $s3_skey --display-name 'M. Tester' --email tester@stone.com -c $conf_fn > /dev/null

    # Create S3-test users
    # See: https://github.com/stone/s3-tests
    debug echo "setting up s3-test users"
    $STONE_BIN/radosgw-admin user create \
        --uid 0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef \
        --access-key ABCDEFGHIJKLMNOPQRST \
        --secret abcdefghijklmnopqrstuvwxyzabcdefghijklmn \
        --display-name youruseridhere \
        --email s3@example.com -c $conf_fn > /dev/null
    $STONE_BIN/radosgw-admin user create \
        --uid 56789abcdef0123456789abcdef0123456789abcdef0123456789abcdef01234 \
        --access-key NOPQRSTUVWXYZABCDEFG \
        --secret nopqrstuvwxyzabcdefghijklmnabcdefghijklm \
        --display-name john.doe \
        --email john.doe@example.com -c $conf_fn > /dev/null
    $STONE_BIN/radosgw-admin user create \
	--tenant testx \
        --uid 9876543210abcdef0123456789abcdef0123456789abcdef0123456789abcdef \
        --access-key HIJKLMNOPQRSTUVWXYZA \
        --secret opqrstuvwxyzabcdefghijklmnopqrstuvwxyzab \
        --display-name tenanteduser \
        --email tenanteduser@example.com -c $conf_fn > /dev/null

    # Create Swift user
    debug echo "setting up user tester"
    $STONE_BIN/radosgw-admin user create -c $conf_fn --subuser=test:tester --display-name=Tester-Subuser --key-type=swift --secret=testing --access=full > /dev/null

    echo ""
    echo "S3 User Info:"
    echo "  access key:  $s3_akey"
    echo "  secret key:  $s3_skey"
    echo ""
    echo "Swift User Info:"
    echo "  account   : test"
    echo "  user      : tester"
    echo "  password  : testing"
    echo ""
}

do_rgw()
{
    if [ "$new" -eq 1 ]; then
        do_rgw_create_users
        if [ -n "$rgw_compression" ]; then
            debug echo "setting compression type=$rgw_compression"
            $STONE_BIN/radosgw-admin zone placement modify -c $conf_fn --rgw-zone=default --placement-id=default-placement --compression=$rgw_compression > /dev/null
        fi
    fi
    # Start server
    if [ "$stoneadm" -gt 0 ]; then
        stone_adm orch apply rgw rgwTest
        return
    fi

    RGWDEBUG=""
    if [ "$debug" -ne 0 ]; then
        RGWDEBUG="--debug-rgw=20 --debug-ms=1"
    fi

    local STONE_RGW_PORT_NUM="${STONE_RGW_PORT}"
    local STONE_RGW_HTTPS="${STONE_RGW_PORT: -1}"
    if [[ "${STONE_RGW_HTTPS}" = "s" ]]; then
        STONE_RGW_PORT_NUM="${STONE_RGW_PORT::-1}"
    else
        STONE_RGW_HTTPS=""
    fi
    RGWSUDO=
    [ $STONE_RGW_PORT_NUM -lt 1024 ] && RGWSUDO=sudo

    current_port=$STONE_RGW_PORT
    for n in $(seq 1 $STONE_NUM_RGW); do
        rgw_name="client.rgw.${current_port}"

        stone_adm auth get-or-create $rgw_name \
            mon 'allow rw' \
            osd 'allow rwx' \
            mgr 'allow rw' \
            >> "$keyring_fn"

        debug echo start rgw on http${STONE_RGW_HTTPS}://localhost:${current_port}
        run 'rgw' $current_port $RGWSUDO $STONE_BIN/radosgw -c $conf_fn \
            --log-file=${STONE_OUT_DIR}/radosgw.${current_port}.log \
            --admin-socket=${STONE_OUT_DIR}/radosgw.${current_port}.asok \
            --pid-file=${STONE_OUT_DIR}/radosgw.${current_port}.pid \
            --rgw_luarocks_location=${STONE_OUT_DIR}/luarocks \
            ${RGWDEBUG} \
            -n ${rgw_name} \
            "--rgw_frontends=${rgw_frontend} port=${current_port}${STONE_RGW_HTTPS}"

        i=$(($i + 1))
        [ $i -eq $STONE_NUM_RGW ] && break

        current_port=$((current_port+1))
    done
}
if [ "$STONE_NUM_RGW" -gt 0 ]; then
    do_rgw
fi

# Ganesha Daemons
if [ $GANESHA_DAEMON_NUM -gt 0 ]; then
    pseudo_path="/stonefs"
    if [ "$stoneadm" -gt 0 ]; then
        cluster_id="vstart"
	port="2049"
        prun stone_adm nfs cluster create $cluster_id
	if [ $STONE_NUM_MDS -gt 0 ]; then
            prun stone_adm nfs export create stonefs --fsname "a" --cluster-id $cluster_id --pseudo-path $pseudo_path
	    echo "Mount using: mount -t nfs -o port=$port $IP:$pseudo_path mountpoint"
	fi
	if [ "$STONE_NUM_RGW" -gt 0 ]; then
            pseudo_path="/rgw"
            do_rgw_create_bucket
	    prun stone_adm nfs export create rgw --cluster-id $cluster_id --pseudo-path $pseudo_path --bucket "nfs-bucket"
            echo "Mount using: mount -t nfs -o port=$port $IP:$pseudo_path mountpoint"
	fi
    else
        start_ganesha
	echo "Mount using: mount -t nfs -o port=<ganesha-port-num> $IP:$pseudo_path mountpoint"
    fi
fi

docker_service(){
     local service=''
     #prefer podman
     if pgrep -f podman > /dev/null; then
	 service="podman"
     elif pgrep -f docker > /dev/null; then
	 service="docker"
     fi
     if [ -n "$service" ]; then
       echo "using $service for deploying jaeger..."
       #check for exited container, remove them and restart container
       if [ "$($service ps -aq -f status=exited -f name=jaeger)" ]; then
	 $service rm jaeger
       fi
       if [ ! "$(podman ps -aq -f name=jaeger)" ]; then
         $service "$@"
       fi
     else
         echo "cannot find docker or podman, please restart service and rerun."
     fi
}

echo ""
if [ $with_jaeger -eq 1 ]; then
    debug echo "Enabling jaegertracing..."
    docker_service run -d --name jaeger \
  -p 5775:5775/udp \
  -p 6831:6831/udp \
  -p 6832:6832/udp \
  -p 5778:5778 \
  -p 16686:16686 \
  -p 14268:14268 \
  -p 14250:14250 \
  jaegertracing/all-in-one:1.20
fi


debug echo "vstart cluster complete. Use stop.sh to stop. See out/* (e.g. 'tail -f out/????') for debug output."

echo ""
if [ "$new" -eq 1 ]; then
    if $with_mgr_dashboard; then
        echo "dashboard urls: $DASH_URLS"
        echo "  w/ user/pass: admin / admin"
    fi
    echo "restful urls: $RESTFUL_URLS"
    echo "  w/ user/pass: admin / $RESTFUL_SECRET"
    echo ""
fi
echo ""
# add header to the environment file
{
    echo "#"
    echo "# source this file into your shell to set up the environment."
    echo "# For example:"
    echo "# $ . $STONE_DIR/vstart_environment.sh"
    echo "#"
} > $STONE_DIR/vstart_environment.sh
{
    echo "export PYTHONPATH=$PYBIND:$CYTHON_PYTHONPATH:$STONE_PYTHON_COMMON\$PYTHONPATH"
    echo "export LD_LIBRARY_PATH=$STONE_LIB:\$LD_LIBRARY_PATH"
    echo "export PATH=$STONE_DIR/bin:\$PATH"

    if [ "$STONE_DIR" != "$PWD" ]; then
        echo "export STONE_CONF=$conf_fn"
        echo "export STONE_KEYRING=$keyring_fn"
    fi

    if [ -n "$STONEFS_SHELL" ]; then
        echo "alias stonefs-shell=$STONEFS_SHELL"
    fi
} | tee -a $STONE_DIR/vstart_environment.sh

echo "STONE_DEV=1"

# always keep this section at the very bottom of this file
STRAY_CONF_PATH="/etc/stone/stone.conf"
if [ -f "$STRAY_CONF_PATH" -a -n "$conf_fn" -a ! "$conf_fn" -ef "$STRAY_CONF_PATH" ]; then
    echo ""
    echo ""
    echo "WARNING:"
    echo "    Please remove stray $STRAY_CONF_PATH if not needed."
    echo "    Your conf files $conf_fn and $STRAY_CONF_PATH may not be in sync"
    echo "    and may lead to undesired results."
    echo ""
    echo "NOTE:"
    echo "    Remember to restart cluster after removing $STRAY_CONF_PATH"
fi

init_logrotate
