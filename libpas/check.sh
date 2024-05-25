#!/bin/sh

set -e
set -x

mkdir -p build
make -f Makefile-macosx check-pas -j `sysctl -n hw.ncpu`

set +x

echo Pizlonator Approves.
