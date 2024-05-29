#!/bin/sh
#
# Copyright (c) 2024 Epic Games, Inc. All Rights Reserved.
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

cp build/lib/libc++.1.0.dylib pizfix/lib
cp build/lib/libc++abi.1.0.dylib pizfix/lib
cp build/lib/libc++.a pizfix/lib
cp build/lib/libc++abi.a pizfix/lib
(cd pizfix/lib &&
     rm -f libc++.1.dylib &&
     rm -f libc++.dylib &&
     rm -f libc++abi.1.dylib &&
     rm -f libc++abi.dylib &&
     ln -s libc++.1.0.dylib libc++.1.dylib &&
     ln -s libc++.1.dylib libc++.dylib &&
     ln -s libc++abi.1.0.dylib libc++abi.1.dylib &&
     ln -s libc++abi.1.dylib libc++abi.dylib)
install_name_tool -id $PWD/pizfix/lib/libc++.1.dylib pizfix/lib/libc++.1.0.dylib
install_name_tool -id $PWD/pizfix/lib/libc++abi.1.dylib \
                  pizfix/lib/libc++abi.1.0.dylib
install_name_tool -change @rpath/libc++abi.1.dylib \
                  $PWD/pizfix/lib/libc++abi.1.dylib \
                  pizfix/lib/libc++.1.0.dylib
