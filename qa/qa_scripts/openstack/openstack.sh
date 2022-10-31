#!/usr/bin/env bash
#
# Install Openstack.
#     Usage: openstack <openstack-site> <stone-monitor>
#
# This script installs Openstack on one node, and connects it to a stone
# cluster on another set of nodes.  It is intended to run from a third
# node.
#
# Assumes a single node Openstack cluster and a single monitor stone
# cluster.
#
# The execs directory contains scripts to be run on remote sites.
# The files directory contains files to be copied to remote sites.
#

set -fv
source ./copy_func.sh
source ./fix_conf_file.sh
openstack_node=${1}
stone_node=${2}
./packstack.sh $openstack_node $stone_node
echo 'done running packstack'
sleep 60
./connectstone.sh $openstack_node $stone_node
echo 'done connecting'
sleep 60
./image_create.sh $openstack_node $stone_node
