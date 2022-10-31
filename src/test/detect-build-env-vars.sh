#!/usr/bin/env bash

if [ -n "$STONE_BUILD_DIR" ] && [ -n "$STONE_ROOT" ] && [ -n "$STONE_BIN" ] && [ -n "$STONE_LIB" ]; then
  echo "Enivronment Variables Already Set"
elif [ -e CMakeCache.txt ]; then
  echo "Environment Variables Not All Set, Detected Build System CMake"
  echo "Setting Environment Variables"
  export STONE_ROOT=`grep stone_SOURCE_DIR CMakeCache.txt | cut -d "=" -f 2`
  export STONE_BUILD_DIR=`pwd`
  export STONE_BIN=$STONE_BUILD_DIR/bin
  export STONE_LIB=$STONE_BUILD_DIR/lib
  export PATH=$STONE_BIN:$PATH
  export LD_LIBRARY_PATH=$STONE_LIB
else
  echo "Please execute this command out of the proper directory"
  exit 1
fi


