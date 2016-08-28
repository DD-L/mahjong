#!/bin/bash
#########################################################################
# File Name:    build.sh
# Author:       D_L
# mail:         deel@d-l.top
# Created Time: 2016/7/15 5:52:43
#########################################################################

set -e
set -x

if  !(test -d contrib/boost-ExtractSourceCode ) 
then
    git submodule init
    git submodule update
fi

if  !(test  -f contrib/boost/boost_1_57_0/boost/success_flag) 
then
    cd contrib/boost-ExtractSourceCode; 
    make
    cd -
    mv contrib/boost-ExtractSourceCode/boost contrib/boost
fi

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
