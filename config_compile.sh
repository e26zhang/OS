#!/bin/bash

# This script for reconfiguring and recompile the kernel

cd os161-1.99
./configure --ostree=$HOME/cs350-os161/root --toolprefix=cs350-
cd kern/conf
./config ASST0
cd ../compile/ASST0
bmake depend
bmake
if [[ $? -ne 0 ]] ; then
    exit 1
fi
bmake install
