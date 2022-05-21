#!/bin/bash

if [ $# -ne 0 -a $# -ne 1 ]
then
    echo "Usage: $0 [all|test|clean]"
    exit 1
fi

OPT=${1}

if [ $# -eq 0 ]
then
    OPT="all"
fi

if [ ${OPT} = "clean" ]
then
    rm -rf ./cmake-build/*
    rm -rf ./log/*
    exit 0
fi

cd cmake-build
if [ ${OPT} = "test" ]
then
    cmake -D test=true -D debug=true -D gcov=true ..

    make
    make test
else
    cmake -D test=false -D debug=true -D gcov=false ..
    make
fi

