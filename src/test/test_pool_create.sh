#!/usr/bin/env bash


#Generic create pool use crush rule  test
#

# Includes
source ../qa/standalone/stone-helpers.sh

function run() {
    local dir=$1
    shift

    export STONE_MON="127.0.0.1:17109" # git grep '\<17109\>' : there must be only one
    export STONE_ARGS
    STONE_ARGS+="--fsid=$(uuidgen) --auth-supported=none "
    STONE_ARGS+="--mon-host=$STONE_MON "

    local funcs=${@:-$(set | sed -n -e 's/^\(TEST_[0-9a-z_]*\) .*/\1/p')}
    for func in $funcs ; do
        $func $dir || return 1
    done
}

function TEST_pool_create() {
    local dir=$1
    setup $dir || return 1
    run_mon $dir a || return 1
    run_osd $dir 0 || return 1
    run_osd $dir 1 || return 1
    run_osd $dir 2 || return 1

    local rulename=testrule
    local poolname=rulepool
    local var=`stone osd crush rule dump|grep -w ruleset|sed -n '$p'|grep -o '[0-9]\+'`
    var=`expr  $var + 1 `
    stone osd getcrushmap -o "$dir/map1"
    crushtool -d "$dir/map1" -o "$dir/map1.txt"

    local minsize=0
    local maxsize=1
    sed -i '/# end crush map/i\rule '$rulename' {\n ruleset \'$var'\n type replicated\n min_size \'$minsize'\n max_size \'$maxsize'\n step take default\n step choose firstn 0 type osd\n step emit\n }\n' "$dir/map1.txt"
    crushtool  -c "$dir/map1.txt" -o "$dir/map1.bin"
    stone osd setcrushmap -i "$dir/map1.bin"
    stone osd pool create $poolname 200 $rulename 2>"$dir/rev"
    local result=$(cat "$dir/rev" | grep "Error EINVAL: pool size")

    if [ "$result" = "" ];
    then
      stone osd pool delete  $poolname $poolname  --yes-i-really-really-mean-it
      stone osd crush rule rm $rulename
      return 1
    fi
    stone osd crush rule rm $rulename
}

main testpoolcreate

