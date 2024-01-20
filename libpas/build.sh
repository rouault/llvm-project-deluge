#!/bin/sh

set -e

mkdir -p ../pizfix/include
mkdir -p ../pizfix/stdfil-include
mkdir -p ../pizfix/builtins-include
cp ../deluge/include/*.h ../pizfix/stdfil-include/
cp ../deluge/builtins/*.h ../pizfix/builtins-include/

mkdir -p ../pizfix/bin
mkdir -p ../pizfix/sbin
mkdir -p ../pizfix/libexec
mkdir -p ../pizfix/share

mkdir -p build/lib
mkdir -p build/lib_test
mkdir -p ../pizfix/lib
mkdir -p ../pizfix/lib_test

rm -f build/deluded-*.o
rm -f ../pizfix/lib/libdeluge.dylib
rm -f ../pizfix/lib_test/libdeluge.dylib
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
    xcrun clang -dynamiclib -o ../pizfix/$libname build/$libname-*.o build/deluded-*.o
}

#do_build test/libpas.dylib ""
#do_build test/libpas_test.dylib "-DENABLE_PAS_TESTING=1"
do_build lib/libdeluge.dylib "-DPAS_DELUGE=1"
do_build lib_test/libdeluge.dylib "-DPAS_DELUGE=1 -DENABLE_PAS_TESTING=1"

xcrun clang -g -O3 -dynamiclib -o ../pizfix/lib/libdeluge_crt.dylib ../deluge/main/main.c -Isrc/libpas -undefined dynamic_lookup -DPAS_DELUGE=1
xcrun clang -g -O3 -dynamiclib -o ../pizfix/lib_test/libdeluge_crt.dylib ../deluge/main/main.c -Isrc/libpas -undefined dynamic_lookup -DPAS_DELUGE=1 -DENABLE_PAS_TESTING=1

echo Pizlonator Approves.
