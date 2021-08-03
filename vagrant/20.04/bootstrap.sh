#!/usr/bin/env bash

# Install dependencies
sudo apt-get update
sudo apt-get install -y clang-11 cmake libboost-dev libgmp-dev zlib1g-dev

# Download clam-prov
git clone git@github.com:SRI-CSL/clam-prov.git

# Build clam-prov
cd clam-prov
mkdir build && cd build
cmake ..
cmake --build . --target clam-seadsa && cmake ..
cmake --build . --target clam-seallvm && cmake ..
cmake --build . --target clam-src && cmake ..
cmake --build . --target crab && cmake ..
cmake --build . --target install
