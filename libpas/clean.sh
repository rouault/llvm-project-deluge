#!/bin/sh

set -e
set -x

make -f Makefile-setup clean
make -f Makefile-macosx clean

set +x

echo Pizlonator Approves.
