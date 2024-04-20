#!/bin/sh

set -e

mkdir -p ../pizfix/include
mkdir -p ../pizfix/stdfil-include
mkdir -p ../pizfix/builtins-include
cp ../filc/include/*.h ../pizfix/stdfil-include/
cp ../filc/builtins/*.h ../pizfix/builtins-include/

mkdir -p ../pizfix/bin
mkdir -p ../pizfix/sbin
mkdir -p ../pizfix/libexec
mkdir -p ../pizfix/share

mkdir -p build/lib
mkdir -p build/lib_test
mkdir -p ../pizfix/lib
mkdir -p ../pizfix/lib_test

rm -f build/pizlonated-*.o
rm -f ../pizfix/lib/libpizlo.dylib
rm -f ../pizfix/lib_test/libpizlo.dylib
for x in ../filc/src/*.c
do
    xcrun ../build/bin/clang -O3 -g -W -Werror -Wno-pointer-to-int-cast -c -o build/pizlonated-`basename $x .c`.o $x &
done

ruby src/libpas/generate_pizlonated_forwarders.rb

do_build() {
    libname=$1
    flags=$2
    rm -f build/$libname-*.o
    for x in src/libpas/*.c
    do
        xcrun clang -g -O3 -W -Werror -c -o build/$libname-`basename $x .c`.o $x $flags &
    done
    wait
    xcrun clang -dynamiclib -install_name $PWD/../pizfix/$libname -o ../pizfix/$libname build/$libname-*.o build/pizlonated-*.o
}

#do_build test/libpas.dylib ""
#do_build test/libpas_test.dylib "-DENABLE_PAS_TESTING=1"
do_build lib/libpizlo.dylib "-DPAS_FILC=1"
do_build lib_test/libpizlo.dylib "-DPAS_FILC=1 -DENABLE_PAS_TESTING=1"

xcrun clang -g -O3 -dynamiclib -install_name $PWD/../pizfix/lib/libfilc_crt.dylib -o ../pizfix/lib/libfilc_crt.dylib ../filc/main/main.c -Isrc/libpas -undefined dynamic_lookup -DPAS_FILC=1
xcrun clang -g -O3 -dynamiclib -install_name $PWD/../pizfix/lib_test/libfilc_crt.dylib -o ../pizfix/lib_test/libfilc_crt.dylib ../filc/main/main.c -Isrc/libpas -undefined dynamic_lookup -DPAS_FILC=1 -DENABLE_PAS_TESTING=1

echo Pizlonator Approves.
