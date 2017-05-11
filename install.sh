#!/usr/bin/env bash

if [ ! -d ./build_NestExceptionDeadlockTest ]; then
	mkdir ./build_NestExceptionDeadlockTest
fi

cd ./build_NestExceptionDeadlockTest
cmake -Dwith-nest=`which nest-config` -DCMAKE_CXX_FLAGS='-Wno-unused-variable -Wno-reorder' ..
make -j4
make install
cd ..