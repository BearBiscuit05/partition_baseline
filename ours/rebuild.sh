#!/bin/bash

mkdir -p build
cd build
cmake ..
#cmake -DDEBUG_MODE=ON ..
make
