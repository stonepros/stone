#!/bin/sh -ex

stone -s
rados lspools
rbd ls
# check that the monitors work
stone osd set nodown
stone osd unset nodown

exit 0
