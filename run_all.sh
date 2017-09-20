#!/bin/bash

# This script will run config_compile, build_userlvl and then sys_conf in that order 

./config_compile.sh
./build_userlvl.sh
./sys_conf.sh
