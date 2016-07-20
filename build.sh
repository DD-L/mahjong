#!/bin/bash
#########################################################################
# File Name:    build.sh
# Author:       D_L
# mail:         deel@d-l.top
# Created Time: 2016/7/15 5:52:43
#########################################################################

set -e
set -x

if [ x$1 = x"clean" ]
then
    rm -rf build
    exit
fi

[ -d build ] || mkdir ./build
cd build
cmake ..
#make
make VERBOSE=1 -j 2
make install

set +x
