#!/bin/sh -e

if [ -n "${VALGRIND}" ]; then
  valgrind ${VALGRIND} --suppressions=${TESTDIR}/valgrind.supp \
    --error-exitcode=1 stone_test_rbd_mirror
else
  stone_test_rbd_mirror
fi
exit 0
