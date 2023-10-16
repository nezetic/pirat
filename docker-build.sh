#!/bin/sh
ARCH=`uname -m`

docker build -t prat .

mkdir -p build
cd build

id=$(docker create prat)
docker cp $id:/prat/build/PRat_patchinit.bin .
docker cp $id:/prat/build/PRat_versio.bin .
docker cp $id:/prat/build/PRat_legio.bin .
if [ "${ARCH}" = 'x86_64' ]; then
    docker cp $id:/root/.Rack2/plugins/PRat-2.0.0-lin.vcvplugin .
fi
docker rm -v $id
