#! /bin/sh

set -xe

CPU_COUNT="$( (sysctl -n hw.ncpu || grep -c '^processor *' /proc/cpuinfo || echo 1) 2>/dev/null)"

rm -rf BD_release BD_debug
mkdir BD_release BD_debug

cd BD_release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j"$CPU_COUNT" all test
cd ..

cd BD_debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j"$CPU_COUNT" all test
