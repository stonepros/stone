#!/bin/sh -ex

stone osd pool create rbd
${PYTHON:-python3} -m nose -v $(dirname $0)/../../../src/test/pybind/test_rados.py "$@"
exit 0
