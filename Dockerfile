FROM ubuntu:22.04

ENV BUILDDIR=/prat
WORKDIR ${BUILDDIR}

RUN apt-get update && apt-get install -y \
     build-essential gcc-arm-none-eabi binutils-arm-none-eabi \
     git cmake jq zstd libgl-dev

COPY *.txt ${BUILDDIR}/

RUN mkdir build && cd build && cmake .. -DDEPS_ONLY=1 && make deps

COPY src ${BUILDDIR}/src/

RUN cd build && cmake .. -DDEPS_ONLY=0 && make test

RUN cd build && make firmware && make firmware_versio && make firmware_legio

RUN cd build && make && make install
