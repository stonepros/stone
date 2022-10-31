#!/usr/bin/env bash
#
# Copyright (C) 2014, 2015 Red Hat <contact@redhat.com>
# Copyright (C) 2013 Cloudwatt <libre.licensing@cloudwatt.com>
#
# Author: Loic Dachary <loic@dachary.org>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU Library Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Library Public License for more details.
#
set -ex
source $(dirname $0)/detect-build-env-vars.sh

test `uname` = FreeBSD

STONE_CLI_TEST_DUP_COMMAND=1 \
MON=1 OSD=3 MDS=0 MGR=1 STONE_PORT=7206 $STONE_ROOT/src/test/vstart_wrapper.sh \
    $STONE_ROOT/qa/workunits/rbd/rbd-ggate.sh \
