#!/bin/sh

set -e

mkdir -p build

do_build() {
    suffix=$1
    flags=$2
    rm -f build/libpas-$suffix-*.o
    for x in src/libpas/*.c
    do
        xcrun clang -g -O3 -W -Werror -c -o build/libpas-$suffix-`basename $x .c`.o $x $flags &
    done
    wait
    xcrun clang -dynamiclib -o build/libpas-$suffix.dylib build/libpas-$suffix-*.o
    rm -f build/test_pas-$suffix-*.o
    for x in src/test/*.cpp src/verifier/*.cpp
    do
        xcrun clang++ -g -O3 -W -Werror -c -o build/test_pas-$suffix-`basename $x .cpp`.o $x $flags -Isrc/libpas -Isrc/verifier -std=c++17 -Wno-unused-parameter -Wno-sign-compare &
    done
    wait
    xcrun clang++ -o build/test_pas-$suffix build/test_pas-$suffix-*.o -Lbuild -lpas-$suffix
}

#do_build release ""
do_build test "-DENABLE_PAS_TESTING=1"

DYLD_LIBRARY_PATH=build build/test_pas-test
#DYLD_LIBRARY_PATH=build build/test_pas-release
