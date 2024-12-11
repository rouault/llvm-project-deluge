#!/bin/sh

apt update
apt install clang cmake autoconf automake autopoint libtool patchelf patch ninja-build wget unzip less swig zlib1g-dev ruby

./build_all_fast.sh
./build_all_slow.sh
