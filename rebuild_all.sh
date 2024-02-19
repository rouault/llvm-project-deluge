#!/bin/sh

set -e
set -x

(cd build && ninja)

(cd libpas && ./build.sh)

(cd musl && make clean && make -j `sysctl -n hw.ncpu` && make install)

deluge/run-tests

(cd zlib-1.3 && make clean && make -j `sysctl -n hw.ncpu` && make install)

(cd openssl-3.2.0 && make clean && make -j `sysctl -n hw.ncpu` && make install_sw && make install_ssldirs)

(cd curl-8.5.0 && make distclean && CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --with-openssl --prefix=$PWD/../pizfix && make -j `sysctl -n hw.ncpu` && make install)

(cd deluded-openssh-portable && make distclean && CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix && make -j `sysctl -n hw.ncpu` && make install)
