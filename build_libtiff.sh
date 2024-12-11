
. libpas/common.sh

set -e
set -x

wget https://download.osgeo.org/libtiff/tiff-4.7.0.tar.gz
tar xvzf tiff-4.7.0.tar.gz
cd tiff-4.7.0
cp ../libtiff.map libtiff

CC="$CCPREFIX$PWD/../build/bin/clang -O2 -I$PWD/../pizfix/include" LD="$CCPREFIX$PWD/../build/bin/clang++" cmake . -GNinja "-DCMAKE_INSTALL_PREFIX=$PWD/../pizfix" "-DCMAKE_PREFIX_PATH=$PWD/../pizfix" -Dtiff-tools=off -Dtiff-tests=off -Dtiff-contrib=off
ninja
ninja install
