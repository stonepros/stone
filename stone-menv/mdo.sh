#!/bin/bash
cmd=`basename $0`
MENV_ROOT=`dirname $0`/..

if [ -f $MENV_ROOT/.menvroot ]; then
    . $MENV_ROOT/.menvroot
fi

[ "$MRUN_STONE_ROOT" == "" ] && MRUN_STONE_ROOT=$HOME/stone

if [ "$MRUN_CLUSTER" == "" ]; then
    ${MRUN_STONE_ROOT}/build/bin/$cmd "$@"
    exit $?
fi

${MRUN_STONE_ROOT}/src/mrun $MRUN_CLUSTER $cmd "$@"
