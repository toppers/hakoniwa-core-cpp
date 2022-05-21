#!/bin/bash

OS_TYPE=`uname`

if [ "${OS_TYPE}" = "Darwin" ]
then
    brew install spdlog
else
    sudo apt install libspdlog-dev
fi


