#!/bin/sh
#
# Copyright (c) 2023-2024 Epic Games, Inc. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

. libpas/common.sh

set -e
set -x

rm -rf pizfix

mkdir -p build

if test $OS = macosx
then
    mkdir -p runtime-build
    
    if test ! -f runtime-build/runtime-build-ok1
    then
        (cd runtime-build &&
             cmake -S ../llvm-project-clean/llvm -B . -G Ninja \
                   -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=RelWithDebInfo \
                   -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_RUNTIMES=compiler-rt &&
             ninja &&
             touch runtime-build-ok1)
    fi
fi

(cd build &&
     cmake -S ../llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang \
           -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON \
           -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
           -DLIBCXXABI_HAS_PTHREAD_API=ON -DLIBCXX_ENABLE_EXCEPTIONS=OFF \
           -DLIBCXXABI_ENABLE_EXCEPTIONS=OFF -DLIBCXX_HAS_PTHREAD_API=ON \
           -DLIBCXX_HAS_MUSL_LIBC=ON -DLLVM_ENABLE_ZSTD=OFF \
           -DLIBCXX_FORCE_LIBCXXABI=ON -DLLVM_TARGETS_TO_BUILD=$LLVMARCH &&
     ninja clang)

(cd libpas && ./build.sh)

(cd musl && \
     CC="$CCPREFIX$PWD/../build/bin/clang" ./configure --target=$ARCH \
         --prefix=$PWD/../pizfix --dylib-opt=$DYLIB_OPT --dylib-ext=$DYLIB_EXT && \
     $MAKE clean && \
     $MAKE -j `sysctl -n hw.ncpu` && \
     $MAKE install)

(cd build && ninja runtimes-clean && ninja runtimes)
./install-cxx-$OS.sh

filc/run-tests

(cd zlib-1.3 &&
     ($MAKE distclean || echo whatever) &&
     CC="$CCPREFIX$PWD/../build/bin/clang" CFLAGS="-O3 -g" ./configure \
         --prefix=$PWD/../pizfix &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install)

(cd openssl-3.2.0 &&
     ($MAKE distclean || echo whatever) &&
     CC="$CCPREFIX$PWD/../build/bin/clang -g -O" ./Configure zlib no-asm no-devcryptoeng \
         --prefix=$PWD/../pizfix &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install_sw &&
     $MAKE install_ssldirs)

(cd curl-8.5.0 &&
     ($MAKE distclean || echo whatever) &&
     CC="$CCPREFIX$PWD/../build/bin/clang -g -O" ./configure --with-openssl \
         --prefix=$PWD/../pizfix &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install)

(cd deluded-openssh-portable &&
     ($MAKE distclean || echo whatever) &&
     autoreconf &&
     CC="$CCPREFIX$PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install)

(cd pcre-8.39 &&
     ($MAKE distclean || echo whatever) &&
     autoreconf &&
     CC="$CCPREFIX$PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix \
       --disable-cpp --enable-pcre16 --enable-pcre32 --enable-unicode-properties \
       --enable-pcregrep-libz &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install)
