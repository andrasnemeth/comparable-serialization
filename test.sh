#!/usr/bin/env bash

gtest_dir="modules/googletest"
gtest_build_dir="$gtest_dir/googletest/build"
gtest_archive="$gtest_build_dir/libgtest.a"

# clone googletest if needed
if [ ! -d "$gtest_dir" ]; then
    git submodule update --init --recursive "$gtest_dir"
fi

# build googletest if needed
if [ ! -f "$gtest_archive" ]; then
   mkdir "$gtest_build_dir"
   pushd "$gtest_build_dir"
   cmake ..
   make
   popd
fi

GTEST_LIB="$gtest_archive" tup unit-test

./unit-test
