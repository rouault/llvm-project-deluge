#!/bin/sh
#
# Copyright (c) 2024 Epic Games, Inc. All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY EPIC GAMES, INC. ``AS IS AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL EPIC GAMES, INC. OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 

. libpas/common.sh

set -e
set -x

build_name=filc-0.666.1-$OS-$ARCH

rm -rf $build_name

mkdir $build_name
cp README.md $build_name/

mkdir -p $build_name/build/bin
cp build/bin/clang-17 $build_name/build/bin/
(cd $build_name/build/bin/ &&
     ln -s clang-17 clang &&
     ln -s clang-17 clang++)

mkdir -p $build_name/build/include/
cp -R build/include/c++ $build_name/build/include/

if test $OS = macosx
then
    mkdir -p $build_name/runtime-build/lib/clang/17/lib/
    cp -R runtime-build/lib/clang/17/lib/darwin $build_name/runtime-build/lib/clang/17/lib/
fi

cp -R pizfix $build_name/
rm $build_name/pizfix/etc/moduli
rm $build_name/pizfix/etc/ssh_host*

sourcedir=$PWD

cd $build_name

echo '#!/bin/sh' > setup.sh
echo 'set -e' >> setup.sh
echo 'set -x' >> setup.sh

case $OS in
    macosx)
        fix_paths()
        {
            filename=$1
            for old_path in `otool -L $filename | awk '{print $1}'`
            do
                if echo $old_path | grep -q $sourcedir
                then
                    new_path=pizfix/lib/`basename $old_path`
                    install_name_tool -change $old_path $new_path $filename
                    echo "install_name_tool -change $new_path \$PWD/$new_path $filename" >> setup.sh
                fi
            done
        }
        
        for dylib in pizfix/lib/*.dylib
        do
            if test ! -L $dylib
            then
                old_name=`otool -L $dylib | head -2 | tail -1 | awk '{print $1}'`
                new_name=pizfix/lib/`basename $old_name`
                install_name_tool -id $new_name $dylib
                echo "install_name_tool -id \$PWD/$new_name $dylib" >> setup.sh
                fix_paths $dylib
            fi
        done
        for binary in pizfix/bin/* pizfix/sbin/* pizfix/libexec/*
        do
            fix_paths $binary
        done
        ccprefix="xcrun "
        ;;
    freebsd)
        for binary in pizfix/lib/*.so pizfix/lib/*.so.* pizfix/bin/* pizfix/sbin/* pizfix/libexec/*
        do
            if test ! -L $binary
            then
                if patchelf --set-rpath pizfix/lib $binary
                then
                    echo "patchelf --set-rpath \$PWD/pizfix/lib $binary" >> setup.sh
                fi
            fi
        done
        ccprefix=""
        ;;
    *)
        echo "Bad OS"
        exit 1
        ;;
esac

echo 'set +x' >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "You are all set. Try compiling something with:"' >> setup.sh
echo 'echo' >> setup.sh
echo "echo \"    ${ccprefix}build/bin/clang -o whatever whatever.c -O -g\"" >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "or:"' >> setup.sh
echo 'echo' >> setup.sh
echo "echo \"    ${ccprefix}build/bin/clang++ -o whatever whatever.cpp -O -g\"" >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "Or, try running something, like:"' >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "    pizfix/bin/curl https://www.google.com/"' >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "or:"' >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "    pizfix/bin/ssh foo@bar.com"' >> setup.sh
echo 'echo' >> setup.sh
echo "echo \"Have fun and thank you for trying $build_name.\"" >> setup.sh

chmod 755 setup.sh

cd ..

tar -cJvf $build_name.tar.xz $build_name

