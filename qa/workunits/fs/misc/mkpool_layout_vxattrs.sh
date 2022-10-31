#!/usr/bin/env bash

set -e

touch foo.$$
stone osd pool create foo.$$ 8
stone fs add_data_pool stonefs foo.$$
setfattr -n stone.file.layout.pool -v foo.$$ foo.$$

# cleanup
rm foo.$$
stone fs rm_data_pool stonefs foo.$$
stone osd pool rm foo.$$ foo.$$ --yes-i-really-really-mean-it

echo OK
