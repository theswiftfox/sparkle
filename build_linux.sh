#!/bin/sh

folder=${PWD##*/}
if [ ! "$folder" = "build" ]; then
    if [ ! -d "build" ]; then
        mkdir build
    fi

    cd build
fi

CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug ..

make -j4

