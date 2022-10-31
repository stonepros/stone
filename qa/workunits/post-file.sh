#!/usr/bin/env bash
set -ex

what="$1"
[ -z "$what" ] && what=/etc/udev/rules.d
sudo stone-post-file -d stone-test-workunit $what

echo OK
