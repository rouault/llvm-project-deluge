
. libpas/common.sh

set -e
set -x

wget https://github.com/facebook/zstd/releases/download/v1.5.6/zstd-1.5.6.tar.gz
tar xvzf zstd-1.5.6.tar.gz
cd zstd-1.5.6

rm lib/decompress/huf_decompress_amd64.S

CC="$CCPREFIX$PWD/../build/bin/clang -I$PWD/../pizfix/include -DDYNAMIC_BMI2=0" make -j$(nproc) prefix=/
CC="$CCPREFIX$PWD/../build/bin/clang -I$PWD/../pizfix/include" make install prefix=/ "DESTDIR=$PWD/../pizfix"
