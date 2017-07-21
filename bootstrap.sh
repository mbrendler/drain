#! /bin/sh

set -xe

rm -rf BD_release BD_debug
mkdir BD_release BD_debug

cd BD_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make all test
cd ..

cd BD_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make all test
