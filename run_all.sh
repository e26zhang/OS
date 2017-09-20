#!/bin/bash

# This script will run config_compile, build_userlvl and then sys_conf in that order 

rm -R root
./config_compile.sh
if [[ $? -ne 0 ]] ; then
   echo 'config_compile.sh has failed'
   exit 1
fi
./build_userlvl.sh
if [[ $? -ne 0 ]] ; then
   echo 'build_userlvl.sh has failed'
   exit 1
fi
./sys_conf.sh
