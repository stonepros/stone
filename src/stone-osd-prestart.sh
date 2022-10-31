#!/bin/sh

if [ `uname` = FreeBSD ]; then
  GETOPT=/usr/local/bin/getopt
else
  GETOPT=getopt
fi

eval set -- "$(${GETOPT} -o i: --long id:,cluster: -- $@)"

while true ; do
	case "$1" in
		-i|--id) id=$2; shift 2 ;;
		--cluster) cluster=$2; shift 2 ;;
		--) shift ; break ;;
	esac
done

if [ -z "$id"  ]; then
    echo "Usage: $0 [OPTIONS]"
    echo "--id/-i ID        set ID portion of my name"
    echo "--cluster NAME    set cluster name (default: stone)"
    exit 1;
fi

data="/var/lib/stone/osd/${cluster:-stone}-$id"

# assert data directory exists - see http://tracker.stone.com/issues/17091
if [ ! -d "$data" ]; then
    echo "OSD data directory $data does not exist; bailing out." 1>&2
    exit 1
fi

journal="$data/journal"

if [ -L "$journal" -a ! -e "$journal" ]; then
    udevadm settle --timeout=5 || :
    if [ -L "$journal" -a ! -e "$journal" ]; then
        echo "stone-osd(${cluster:-stone}-$id): journal not present, not starting yet." 1>&2
        exit 0
    fi
fi

# ensure ownership is correct
owner=`stat -c %U $data/.`
if [ $owner != 'stone' -a $owner != 'root' ]; then
    echo "stone-osd data dir $data is not owned by 'stone' or 'root'"
    echo "you must 'chown -R stone:stone ...' or similar to fix ownership"
    exit 1
fi

exit 0
