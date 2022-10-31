#!/bin/sh -ex

$STONE_TOOL mds tell 0 injectargs '--mds-bal-interval 0'
$STONE_TOOL mds tell 1 injectargs '--mds-bal-interval 0'
$STONE_TOOL mds tell 2 injectargs '--mds-bal-interval 0'
$STONE_TOOL mds tell 3 injectargs '--mds-bal-interval 0'
#$STONE_TOOL mds tell 4 injectargs '--mds-bal-interval 0'

mkdir -p ./a/a
mkdir -p ./b/b
mkdir -p ./c/c
mkdir -p ./d/d

mount_dir=`df . | grep -o " /.*" | grep -o "/.*"`
cur_dir=`pwd`
stone_dir=${cur_dir##$mount_dir}
$STONE_TOOL mds tell 0 export_dir $stone_dir/b 1
$STONE_TOOL mds tell 0 export_dir $stone_dir/c 2
$STONE_TOOL mds tell 0 export_dir $stone_dir/d 3
sleep 5

