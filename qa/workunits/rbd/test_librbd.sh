#!/bin/sh -e

if [ -n "${VALGRIND}" ]; then
  valgrind ${VALGRIND} --suppressions=${TESTDIR}/valgrind.supp \
    --error-exitcode=1 stone_test_librbd
else
  stone_test_librbd
fi
exit 0
