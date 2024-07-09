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

# Stash filbsdrt instead of deleting it, in case we want to quickly recover it.
if test -d filbsdrt
then
    rm -rf filbsdrt-saved
    mv filbsdrt filbsdrt-saved
fi

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

./configure_llvm.sh
./build_base.sh

./build_zlib.sh
./build_bzip2.sh
./build_xz.sh
./build_openssl.sh
./build_curl.sh
./build_openssh.sh
./build_pcre.sh
./build_jpeg-6b.sh
./build_mg.sh
./build_sqlite.sh
