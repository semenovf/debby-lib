#!/bin/bash

CWD=`pwd`
ROCKSDB_RELEASE=v6.29.5

if [ -e .git ] ; then

    git checkout master && git pull origin master \
        && git submodule update --init \
        && git submodule update --remote \
        && cd 3rdparty/portable-target && git checkout master && git pull origin master \
        && cd $CWD \
        && cd 3rdparty/pfs/common && ./update.sh \
        && cd $CWD \
        && cd 3rdparty/rocksdb && git checkout $ROCKSDB_RELEASE

fi

