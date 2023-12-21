#!/bin/sh

set -e

do_build() {
    libname=$1
    flags=$2
    mkdir -p build
    rm -f build/*.o
    for x in src/libpas/*.c
    do
        xcrun clang -O3 -W -Werror -c -o build/`basename $x .c`.o -I../deluge/include $x $flags &
    done
    wait
    xcrun clang -dynamiclib -o build/$libname build/*.o
}

do_build libpas_test.dylib "-DENABLE_PAS_TESTING=1"
do_build libpas_deluge.dylib "-DPAS_DELUGE=1"

