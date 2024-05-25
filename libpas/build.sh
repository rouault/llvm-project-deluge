#!/bin/sh

. common.sh

set -e
set -x

mkdir -p build
mkdir -p ../pizfix/include
mkdir -p ../pizfix/stdfil-include
mkdir -p ../pizfix/builtins-include
mkdir -p ../pizfix/bin
mkdir -p ../pizfix/sbin
mkdir -p ../pizfix/libexec
mkdir -p ../pizfix/share
mkdir -p ../pizfix/lib
mkdir -p ../pizfix/lib_test
$MAKE -f Makefile-setup
$MAKE -f Makefile-$OS -j `sysctl -n hw.ncpu`

set +x

echo Pizlonator Approves.
