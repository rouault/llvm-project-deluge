#!/bin/sh

set -e

mkdir -p build
mkdir -p runtime-build

(cd build && cmake -S ../llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_RUNTIMES= && ninja)

(cd runtime-build && cmake -S ../../llvm-project-clean/llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_RUNTIMES=compiler-rt && ninja)

(cd libpas && ./build.sh)

(cd musl && CC=../build/bin/clang ./configure --target=aarch64 --prefix=prefix && make -j `sysctl -n hw.ncpu` && make install-headers)

(cd zlib-1.3 && CC="xcrun ../build/bin/clang" CFLAGS="-O3 -g" ./configure && make -j `sysctl -n hw.ncpu`)

(cd openssl-3.2.0 && CC="xcrun ../build/bin/clang -g -O" CPPFLAGS=-I../zlib-1.3/ LDFLAGS=-L../zlib-1.3 ./Configure zlib no-asm && make -j `sysctl -n hw.ncpu`)
