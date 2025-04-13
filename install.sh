#!/bin/bash

set -x

g++ c2b.cpp -c -o libc2b.a -O1

if [ $? -ne 0 ]; then
    echo "Error: Failed to compile c2b.cpp"
    exit 1
fi

g++ binaries/c2b_installer.cpp -o binaries/installer -O1 -L. -lc2b

cd binaries
sudo ./installer install
