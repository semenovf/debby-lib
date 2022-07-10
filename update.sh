#!/bin/bash

CWD=`pwd`
ROCKSDB_RELEASE=v6.29.5

if [ -d .git ] ; then

    git pull \
        && git submodule update --init \
        && git submodule update --remote \
        && cd 3rdparty/portable-target && git checkout master && git pull \
        && cd $CWD \
        && cd 3rdparty/pfs/common && git checkout master && git pull  \
        && cd $CWD \
        && cd 3rdparty/rocksdb && git checkout $ROCKSDB_RELEASE

fi

