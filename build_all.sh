#!/bin/sh

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
           -DLIBCXX_DEFAULT_ABI_LIBRARY=libcxxabi &&
     ninja clang)

(cd libpas && ./build.sh)

(cd musl && \
     CC="xcrun $PWD/../build/bin/clang" ./configure --target=aarch64 \
         --prefix=$PWD/../pizfix --dylib-opt=-dynamiclib --dylib-ext=dylib && \
     make clean && \
     make -j `sysctl -n hw.ncpu` && \
     make install)

(cd build && ninja runtimes-clean && ninja runtimes)
./install-cxx-darwin.sh

filc/run-tests

(cd zlib-1.3 &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang" CFLAGS="-O3 -g" ./configure \
         --prefix=$PWD/../pizfix &&
     make -j `sysctl -n hw.ncpu` &&
     make install)

(cd openssl-3.2.0 &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang -g -O" ./Configure zlib no-asm \
         --prefix=$PWD/../pizfix &&
     make -j `sysctl -n hw.ncpu` &&
     make install_sw &&
     make install_ssldirs)

(cd curl-8.5.0 &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --with-openssl \
         --prefix=$PWD/../pizfix &&
     make -j `sysctl -n hw.ncpu` &&
     make install)

(cd deluded-openssh-portable &&
     (if test ! -f configure
      then
          autoreconf
      fi) &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix &&
     make -j `sysctl -n hw.ncpu` &&
     make install)

(cd pcre-8.39 &&
     (make distclean || echo whatever) &&
     CC="xcrun $PWD/../build/bin/clang -g -O" ./configure --prefix=$PWD/../pizfix \
       --disable-cpp --enable-pcre16 --enable-pcre32 --enable-unicode-properties \
       --enable-pcregrep-libz &&
     make -j `sysctl -n hw.ncpu` &&
     make install)
