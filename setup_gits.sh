#!/bin/sh

set -e
set -x

(git clone . llvm-project-clean && cd llvm-project-clean && git checkout 6009708b4367171ccdbf4b5905cb6a803753fe18)

git clone https://github.com/pizlonator/deluded-musl musl

git clone https://github.com/pizlonator/deluded-zlib-1.3.git zlib-1.3
git clone https://github.com/pizlonator/deluded-openssl-3.2.0.git openssl-3.2.0
git clone https://github.com/pizlonator/deluded-curl-8.5.0.git curl-8.5.0
git clone https://github.com/pizlonator/deluded-openssh-portable.git
git clone https://github.com/pizlonator/pizlonated-pcre-8.39.git pcre-8.39

