#!/usr/bin/env bash
#
# Install a simple stone cluster upon which openstack images will be stored.
#
set -fv
stone_node=${1}
source copy_func.sh
copy_file files/$OS_STONE_ISO $stone_node .
copy_file execs/stone_cluster.sh $stone_node . 0777 
copy_file execs/stone-pool-create.sh $stone_node . 0777
ssh $stone_node ./stone_cluster.sh $*
