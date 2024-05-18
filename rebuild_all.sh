#!/bin/sh

set -e
set -x

rm -rf pizfix

(cd build && ninja)

(cd libpas && ./build.sh)

(cd musl && make clean && make -j `sysctl -n hw.ncpu` && make install)

filc/run-tests

(cd zlib-1.3 &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang" CFLAGS="-O3 -g" ./configure --prefix=$PWD/../pizfix &&
     make -j `sysctl -n hw.ncpu` &&
     make install)

(cd openssl-3.2.0 &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang -g -O" ./Configure zlib no-asm --prefix=$PWD/../pizfix &&
     make -j `sysctl -n hw.ncpu` &&
     make install_sw &&
     make install_ssldirs)

(cd curl-8.5.0 &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --with-openssl --prefix=$PWD/../pizfix &&
     make -j `sysctl -n hw.ncpu` &&
     make install)

(cd deluded-openssh-portable &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix &&
     make -j `sysctl -n hw.ncpu` &&
     make install)

(cd pcre-8.39 &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix --disable-cpp --enable-pcre16 --enable-pcre32 --enable-unicode-properties --enable-pcregrep-libz &&
     make -j `sysctl -n hw.ncpu` &&
     make install)
