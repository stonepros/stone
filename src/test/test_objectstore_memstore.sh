#!/bin/sh -ex

rm -rf memstore.test_temp_dir
stone_test_objectstore --gtest_filter=\*/0

echo OK
