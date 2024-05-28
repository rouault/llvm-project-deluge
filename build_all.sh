#!/bin/sh

. libpas/common.sh

set -e
set -x

rm -rf pizfix

mkdir -p build
mkdir -p runtime-build

if test ! -f runtime-build/runtime-build-ok1
then
   (cd runtime-build &&
        cmake -S ../llvm-project-clean/llvm -B . -G Ninja \
              -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=RelWithDebInfo \
              -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_RUNTIMES=compiler-rt &&
        ninja &&
        touch runtime-build-ok1)
fi

(cd build &&
     cmake -S ../llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang \
           -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON \
           -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
           -DLIBCXXABI_HAS_PTHREAD_API=ON -DLIBCXX_ENABLE_EXCEPTIONS=OFF \
           -DLIBCXXABI_ENABLE_EXCEPTIONS=OFF -DLIBCXX_HAS_PTHREAD_API=ON \
           -DLIBCXX_HAS_MUSL_LIBC=ON -DLLVM_ENABLE_ZSTD=OFF \
           -DLIBCXX_FORCE_LIBCXXABI=ON &&
     ninja clang)

(cd libpas && ./build.sh)

(cd musl && \
     CC="$CCPREFIX$PWD/../build/bin/clang" ./configure --target=$ARCH \
         --prefix=$PWD/../pizfix --dylib-opt=$DYLIB_OPT --dylib-ext=$DYLIB_EXT && \
     $MAKE clean && \
     $MAKE -j `sysctl -n hw.ncpu` && \
     $MAKE install)

(cd build && ninja runtimes-clean && ninja runtimes)
./install-cxx-$OS.sh

filc/run-tests

(cd zlib-1.3 &&
     ($MAKE distclean || echo whatever) &&
     CC="$CCPREFIX$PWD/../build/bin/clang" CFLAGS="-O3 -g" ./configure \
         --prefix=$PWD/../pizfix &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install)

(cd openssl-3.2.0 &&
     ($MAKE distclean || echo whatever) &&
     CC="$CCPREFIX$PWD/../build/bin/clang -g -O" ./Configure zlib no-asm \
         --prefix=$PWD/../pizfix &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install_sw &&
     $MAKE install_ssldirs)

(cd curl-8.5.0 &&
     ($MAKE distclean || echo whatever) &&
     CC="$CCPREFIX$PWD/../build/bin/clang -g -O" ./configure --with-openssl \
         --prefix=$PWD/../pizfix &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install)

(cd deluded-openssh-portable &&
     (if test ! -f configure
      then
          autoreconf
      fi) &&
     ($MAKE distclean || echo whatever) &&
     CC="$CCPREFIX$PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install)

(cd pcre-8.39 &&
     ($MAKE distclean || echo whatever) &&
     CC="$CCPREFIX$PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix \
       --disable-cpp --enable-pcre16 --enable-pcre32 --enable-unicode-properties \
       --enable-pcregrep-libz &&
     $MAKE -j `sysctl -n hw.ncpu` &&
     $MAKE install)
