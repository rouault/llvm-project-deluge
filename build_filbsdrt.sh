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

set -e
set -x

rm -rf filbsdrt

# Stash pizfix instead of deleting it, in case we want to quickly recover it.
if test -d pizfix
then
    rm -rf pizfix-saved
    mv pizfix pizfix-saved
fi

(cd build &&
     cmake -S ../llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang \
           -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON \
           -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
           -DLIBCXXABI_HAS_PTHREAD_API=ON -DLIBCXX_ENABLE_EXCEPTIONS=OFF \
           -DLIBCXXABI_ENABLE_EXCEPTIONS=OFF -DLIBCXX_HAS_PTHREAD_API=ON \
           -DLIBCXX_HAS_MUSL_LIBC=ON -DLLVM_ENABLE_ZSTD=OFF \
           -DLIBCXX_FORCE_LIBCXXABI=ON -DLLVM_TARGETS_TO_BUILD=X86 &&
     ninja clang)

(cd libpas && ./build-filbsd.sh)

