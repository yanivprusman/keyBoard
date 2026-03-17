#!/bin/bash
set -e
if [[ " $@ " =~ " -rebuild " ]]; then
    rm -rf build
fi
if [ ! -d "build" ]; then
    mkdir -p build
fi
cd build
cmake .. && \
make -j$(nproc) && \
echo -e "\033[0;32mBuild complete!\033[0m"
cd ..
