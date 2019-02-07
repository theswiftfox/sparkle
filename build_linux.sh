#!/bin/sh

if [ ! -d "build" ]; then
    mkdir build
fi

cd build

CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug ..

make -j4

