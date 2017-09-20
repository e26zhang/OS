#!/bin/bash

# This script is the build the user level programs of the OS

cd $HOME/cs350-os161/os161-1.99 
bmake
if [[ $? -ne 0 ]] ; then
    exit 1
fi
bmake install
