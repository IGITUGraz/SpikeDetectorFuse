#!/usr/bin/env bash

if [ ! -d ./build_SpikeDetectorFuse ]; then
	mkdir ./build_SpikeDetectorFuse
fi

cd ./build_SpikeDetectorFuse
cmake -Dwith-nest=`which nest-config` -DCMAKE_CXX_FLAGS='-std=c++11 -Wno-unused-variable -Wno-reorder' ..
make -j4
make install
cd ..