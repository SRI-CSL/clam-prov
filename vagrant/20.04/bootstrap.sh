#!/usr/bin/env bash

# Note: Since clam-prov is a private repository, for Vagrant provisioning:
# - Your ssh public key should be here: https://github.com/settings/keys
# - ssh-agent should be running: https://www.ssh.com/academy/ssh/agent
# - Your ssh private key should be added to the agent: e.g. ssh-add -K ~/.ssh/id_rsa
# - To check that it is availble: ssh-add -l

# Install dependencies
sudo apt-get update
sudo apt-get install -y clang-11 cmake libboost-dev libgmp-dev zlib1g-dev

# Download clam-prov
mkdir -p ~/.ssh
chmod 700 ~/.ssh
ssh-keyscan -H github.com >> ~/.ssh/known_hosts
ssh -vT git@github.com
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
