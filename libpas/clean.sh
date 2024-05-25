#!/bin/sh

. common.sh

set -e
set -x

$MAKE -f Makefile-setup clean
$MAKE -f Makefile-$OS clean

set +x

echo Pizlonator Approves.
