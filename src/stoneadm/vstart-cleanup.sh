#!/bin/sh -ex

bin/stone mon rm `hostname`
for f in `bin/stone orch ls | grep -v NAME | awk '{print $1}'` ; do
    bin/stone orch rm $f --force
done
