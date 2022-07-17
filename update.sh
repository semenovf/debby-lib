#!/bin/bash

CWD=`pwd`
ROCKSDB_RELEASE=v6.29.5

if [ -e .git ] ; then

    git checkout master && git pull origin master \
        && git submodule update --init --recursive \
        && git submodule update --init --remote -- 3rdparty/portable-target \
        && git submodule update --init --remote -- 3rdparty/pfs/common \
        && cd 3rdparty/rocksdb && git checkout $ROCKSDB_RELEASE \        
        && cd $CWD

fi

