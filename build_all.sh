#!/bin/sh

set -e

mkdir -p build
mkdir -p runtime-build

(cd build && cmake -S ../llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_RUNTIMES= && ninja)

(cd runtime-build && cmake -S ../../llvm-project-clean/llvm -B . -G Ninja -DLLVM_ENABLE_PROJECTS=clang -DCMAKE_BUILD_TYPE=RelWithDebInfo -DLLVM_ENABLE_ASSERTIONS=ON -DLLVM_ENABLE_RUNTIMES=compiler-rt && ninja)

(cd libpas && ./build.sh)
