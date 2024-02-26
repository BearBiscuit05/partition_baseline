#!/bin/bash
rm -rf build
mkdir -p build
cd build
cmake ..
# cmake -DDEBUG_MODE=ON ..
make -j8
