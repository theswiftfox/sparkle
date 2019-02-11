#!/bin/sh

folder=${PWD##*/}
if [ ! "$folder" = "build" ]; then
    if [ ! -d "build" ]; then
        mkdir build
    fi

    cd build
fi

if [ ! -d "shaders" ]; then
    mkdir shaders
fi

CC=clang CXX=clang++ cmake -DCMAKE_BUILD_TYPE=Debug ..

make -j4

