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

case $OS in
    macosx)
        MUSL_DYLIB_OPT=-dynamiclib
        MUSL_DYLIB_EXT=dylib
        MUSL_PREFIX=pizlonated_
        ;;
    freebsd)
        MUSL_DYLIB_OPT="-shared -Wl,-soname,libc.so.666"
        MUSL_DYLIB_EXT=so.666
        MUSL_PREFIX=
        ;;
    openbsd)
        MUSL_DYLIB_OPT=-shared
        MUSL_DYLIB_EXT=so
        MUSL_PREFIX=pizlonated_
        ;;
    linux)
        MUSL_DYLIB_OPT=-shared
        MUSL_DYLIB_EXT=so
        MUSL_PREFIX=
        ;;
    *)
        echo "Should not get here"
        exit 1
        ;;
esac

(cd musl && \
     CC="$CCPREFIX$PWD/../build/bin/clang" ./configure --target=$ARCH \
         --prefix=$PWD/../pizfix --dylib-opt="$MUSL_DYLIB_OPT" \
         --dylib-ext=$MUSL_DYLIB_EXT --libc-prefix=$MUSL_PREFIX && \
     $MAKE clean && \
     $MAKE -j $NCPU && \
     $MAKE install)

case $OS in
    freebsd)
        rm -f pizfix/lib/libc.so
        (cd pizfix/lib && ln -s libc.so.666 libc.so)
        ;;
    linux)
        ar rc pizfix/lib/libm.a
        ar rc pizfix/lib/librt.a
        ar rc pizfix/lib/libpthread.a
        ar rc pizfix/lib/libcrypt.a
        ar rc pizfix/lib/libutil.a
        ar rc pizfix/lib/libxnet.a
        ar rc pizfix/lib/libresolv.a
        ar rc pizfix/lib/libdl.a
        ;;
    *)
        ;;
esac


