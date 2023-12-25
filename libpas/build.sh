#!/bin/sh

set -e

mkdir -p build/test

do_build() {
    libname=$1
    flags=$2
    mkdir -p build
    rm -f build/*.o
    for x in src/libpas/*.c
    do
        xcrun clang -O3 -W -Werror -c -o build/`basename $x .c`.o $x $flags &
    done
    wait
    xcrun clang -dynamiclib -o build/$libname build/*.o
}

do_build test/libpas.dylib ""
do_build test/libpas_test.dylib "-DENABLE_PAS_TESTING=1"
do_build libdeluge.dylib "-DPAS_DELUGE=1"
do_build test/libdeluge.dylib "-DPAS_DELUGE=1 -DENABLE_PAS_TESTING=1"

