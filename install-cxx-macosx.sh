#!/bin/sh

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
