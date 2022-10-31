#!/bin/sh -ex

TMPDIR=/tmp/test_stone-erasure-code-tool.$$
mkdir $TMPDIR
trap "rm -fr $TMPDIR" 0

stone-erasure-code-tool test-plugin-exists INVALID_PLUGIN && exit 1
stone-erasure-code-tool test-plugin-exists jerasure

stone-erasure-code-tool validate-profile \
                       plugin=jerasure,technique=reed_sol_van,k=2,m=1

test "$(stone-erasure-code-tool validate-profile \
          plugin=jerasure,technique=reed_sol_van,k=2,m=1 chunk_count)" = 3

test "$(stone-erasure-code-tool calc-chunk-size \
          plugin=jerasure,technique=reed_sol_van,k=2,m=1 4194304)" = 2097152

dd if="$(which stone-erasure-code-tool)" of=$TMPDIR/data bs=770808 count=1
cp $TMPDIR/data $TMPDIR/data.orig

stone-erasure-code-tool encode \
                       plugin=jerasure,technique=reed_sol_van,k=2,m=1 \
                       4096 \
                       0,1,2 \
                       $TMPDIR/data
test -f $TMPDIR/data.0
test -f $TMPDIR/data.1
test -f $TMPDIR/data.2

rm $TMPDIR/data

stone-erasure-code-tool decode \
                       plugin=jerasure,technique=reed_sol_van,k=2,m=1 \
                       4096 \
                       0,2 \
                       $TMPDIR/data

size=$(stat -c '%s' $TMPDIR/data.orig)
truncate -s "${size}" $TMPDIR/data # remove stripe width padding
cmp $TMPDIR/data.orig $TMPDIR/data

echo OK
