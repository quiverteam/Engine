#!/bin/bash
set -e 
pushd $(dirname $0)/../src > /dev/null
rm -rf build-mingw32
mkdir -p build-mingw32
cd build-mingw32 
# Configure cmake
#cmake ../ -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DUNIX_CROSS=ON
cmake ../ -DCMAKE_C_COMPILER=i686-w64-mingw32-gcc -DCMAKE_CXX_COMPILER=i686-w64-mingw32-g++ -DUNIX_CROSS=1 -DCMAKE_C_FLAGS=-m32 -DCMAKE_CXX_FLAGS=-m32 -DMALLOC_OVERRIDE=1 -DCMAKE_TOOLCHAIN_FILE=cmake_scripts/mingw32-toolchain.cmake
popd > /dev/null 
