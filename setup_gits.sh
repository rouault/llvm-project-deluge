#!/bin/sh

set -e
set -x

(git clone . llvm-project-clean && cd llvm-project-clean && git checkout 6009708b4367171ccdbf4b5905cb6a803753fe18)

git clone ssh://www.filpizlo.com/home/pizlo/deluge-musl musl

git clone ssh://www.filpizlo.com/home/pizlo/zlib-1.3
git clone ssh://www.filpizlo.com/home/pizlo/openssl-3.2.0
