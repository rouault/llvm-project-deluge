#!/bin/sh

set -e
set -x

build_name=filc-0.666

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

mkdir -p $build_name/runtime-build/lib/clang/17/lib/
cp -R runtime-build/lib/clang/17/lib/darwin $build_name/runtime-build/lib/clang/17/lib/

cp -R pizfix $build_name/
rm $build_name/pizfix/etc/moduli
rm $build_name/pizfix/etc/ssh_host*

sourcedir=$PWD

cd $build_name

echo '#!/bin/sh' > setup.sh
echo 'set -e' >> setup.sh
echo 'set -x' >> setup.sh

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

echo 'set +x' >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "You are all set. Try compiling something with:"' >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "    xcrun build/bin/clang -o whatever whatever.c -O -g"' >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "or:"' >> setup.sh
echo 'echo' >> setup.sh
echo 'echo "    xcrun build/bin/clang++ -o whatever whatever.cpp -O -g -fno-exceptions"' >> setup.sh
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

tar -czvf $build_name.tar.gz $build_name

