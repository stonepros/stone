#!/bin/sh -e

stone_test_libstonefs
stone_test_libstonefs_access
stone_test_libstonefs_reclaim
stone_test_libstonefs_lazyio

exit 0
