#!/bin/sh
ARCH=`uname -m`

docker build -t pirat .

mkdir -p build
cd build

id=$(docker create pirat)
docker cp $id:/pirat/build/PiRAT_patchinit.bin .
docker cp $id:/pirat/build/PiRAT_versio.bin .
docker cp $id:/pirat/build/PiRAT_legio.bin .
if [ "${ARCH}" = 'x86_64' ]; then
    docker cp $id:/root/.Rack2/plugins/PiRAT-2.0.0-lin.vcvplugin .
fi
docker rm -v $id
