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

OS_TYPE="posix"
if [ `uname` = "Darwin" ]
then
    :
else
    uname -a | grep Linux > /dev/null
    if [ $? -ne 0 ]
    then
        OS_TYPE="win"
    fi
fi

mkdir -p cmake-build
cd cmake-build
if [ ${OPT} = "test" ]
then
    if [ ${OS_TYPE} = "posix" ]
    then
        cmake -D test=true -D debug=true -D gcov=true .. ${OS_OPT}
    else
        cmake -G "Unix Makefiles" -D WIN32=true -D test=true -D debug=true -D gcov=true .. ${OS_OPT}
    fi

    make
    make test
else
    if [ ${OS_TYPE} = "posix" ]
    then
        cmake -D test=false -D debug=true -D gcov=false ..
    else
        cmake .. -G "Unix Makefiles" -D WIN32=true
    fi
    make
fi

