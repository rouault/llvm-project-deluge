
. libpas/common.sh

set -e
set -x

wget https://download.osgeo.org/proj/proj-9.5.1.tar.gz
tar xvzf proj-9.5.1.tar.gz
cd proj-9.5.1

# Get the SQLite build to use our tclsh.
export PATH=$PWD/pizfix/bin:$PATH

mkdir -p build_proj_pizfix

CC="$CCPREFIX$PWD/../build/bin/clang -I$PWD/../pizfix/include" LD="$CCPREFIX$PWD/../build/bin/clang++" cmake -B build_proj_pizfix -S /home/even/proj/proj -GNinja "-DCMAKE_INSTALL_PREFIX=$PWD/../pizfix" "-DCMAKE_PREFIX_PATH=$PWD/../pizfix" -DBUILD_TESTING=OFF -DCMAKE_BUILD_TYPE=Release

cd build_proj_pizfix
ninja
ninja install
