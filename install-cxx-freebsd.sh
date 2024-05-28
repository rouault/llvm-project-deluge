#!/bin/sh

set -e
set -x

cp build/lib/x86_64-unknown-freebsd14.0/libc++.so pizfix/lib
cp build/lib/x86_64-unknown-freebsd14.0/libc++.so.1.0 pizfix/lib
cp build/lib/x86_64-unknown-freebsd14.0/libc++abi.so.1.0 pizfix/lib
cp build/lib/x86_64-unknown-freebsd14.0/libc++.a pizfix/lib
cp build/lib/x86_64-unknown-freebsd14.0/libc++abi.a pizfix/lib
(cd pizfix/lib &&
     rm -f libc++.so.1 &&
     rm -f libc++abi.so.1 &&
     rm -f libc++abi.so &&
     ln -s libc++.so.1.0 libc++.so.1 &&
     ln -s libc++abi.so.1.0 libc++abi.so.1 &&
     ln -s libc++abi.so.1 libc++abi.so)
