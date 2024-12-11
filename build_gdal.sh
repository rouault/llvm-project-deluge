
. libpas/common.sh

set -e
set -x

if ! test -d gdal-deluge; then
    wget https://github.com/rouault/gdal/archive/refs/heads/deluge.zip
    unzip deluge.zip
fi

ln -sf clang $PWD/build/bin/cc
ln -sf clang++ $PWD/build/bin/c++

mkdir -p build_gdal_pizfix

CC="$CCPREFIX$PWD/build/bin/clang -I$PWD/pizfix/include" CXX="$CCPREFIX$PWD/build/bin/clang++ -I$PWD/pizfix/include" LD="$CCPREFIX$PWD/build/bin/clang++" cmake  -B build_gdal_pizfix -S gdal-deluge -GNinja "-DCMAKE_INSTALL_PREFIX=$PWD/pizfix" "-DCMAKE_PREFIX_PATH=$PWD/pizfix" -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release -DBUILD_PYTHON_BINDINGS=OFF

# Python bindings build requires setuptools, but ./pizfix/bin/python3 -m pip install setuptools doesn't work
# currently due to __clock_nanosleep() not implemented. One can workaround that
# by manually copying from a working installation.
#-DPython_LOOKUP_VERSION=3.13.0 -DBUILD_PYTHON_BINDINGS=ON

cd build_gdal_pizfix

ninja
ninja install
