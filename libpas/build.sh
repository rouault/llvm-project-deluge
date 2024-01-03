#!/bin/sh

set -e

mkdir -p build/test

rm -f build/deluded-*.o
for x in ../deluge/src/*.c
do
    xcrun ../build/bin/clang -O3 -g -W -Werror -Wno-pointer-to-int-cast -c -o build/deluded-`basename $x .c`.o $x &
done

do_build() {
    libname=$1
    flags=$2
    rm -f build/$libname-*.o
    for x in src/libpas/*.c
    do
        xcrun clang -g -O3 -W -Werror -c -o build/$libname-`basename $x .c`.o $x $flags &
    done
    wait
    xcrun clang -dynamiclib -o build/$libname build/$libname-*.o build/deluded-*.o
}

do_build test/libpas.dylib ""
do_build test/libpas_test.dylib "-DENABLE_PAS_TESTING=1"
do_build libdeluge.dylib "-DPAS_DELUGE=1"
do_build test/libdeluge.dylib "-DPAS_DELUGE=1 -DENABLE_PAS_TESTING=1"

xcrun clang -g -O3 -dynamiclib -o build/libdeluge_crt.dylib ../deluge/main/main.c -Isrc/libpas -undefined dynamic_lookup -DPAS_DELUGE=1
xcrun clang -g -O3 -dynamiclib -o build/test/libdeluge_crt.dylib ../deluge/main/main.c -Isrc/libpas -undefined dynamic_lookup -DPAS_DELUGE=1 -DENABLE_PAS_TESTING=1
