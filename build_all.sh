#!/bin/sh

set -e
set -x

mkdir -p build
mkdir -p runtime-build

(cd build && cmake -S ../llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_RUNTIMES= && ninja)

(cd runtime-build && cmake -S ../llvm-project-clean/llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_RUNTIMES=compiler-rt && ninja)

(cd libpas && ./build.sh)

(cd musl && CC=$PWD/../build/bin/clang ./configure --target=aarch64 --prefix=$PWD/../pizfix && make -j `sysctl -n hw.ncpu` && make install)

deluge/run-tests

(cd zlib-1.3 && CC="xcrun $PWD/../build/bin/clang" CFLAGS="-O3 -g" ./configure --prefix=$PWD/../pizfix && make -j `sysctl -n hw.ncpu` && make install)

(cd openssl-3.2.0 && CC="xcrun $PWD/../build/bin/clang -g -O" ./Configure zlib no-asm --prefix=$PWD/../pizfix && make -j `sysctl -n hw.ncpu` && make install)

(cd curl-8.5.0 && CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --with-openssl --prefix=$PWD/../pizfix && make -j `sysctl -n hw.ncpu` && make install)

(cd deluded-openssh-portable && autoreconf && CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix && make -j `sysctl -n hw.ncpu` && make install)
