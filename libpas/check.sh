#!/bin/sh

. common.sh

set -e
set -x

mkdir -p build
$MAKE -f Makefile-$OS check-pas -j `sysctl -n hw.ncpu`

set +x

echo Pizlonator Approves.
