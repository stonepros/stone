#!/bin/bash -e

# fsid
if [ -e fsid ] ; then
    fsid=`cat fsid`
else
    echo 'no fsid file, so no cluster?'
    exit 0
fi
echo "fsid $fsid"

sudo ../src/stoneadm/stoneadm rm-cluster --force --fsid $fsid

