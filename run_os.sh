#!/bin/bash

# This script is for running the OS

cd ./root/

if [ "$1" -eq 0 ]
then
#	sys161 kernel "p uw-testbin/onefork;q"
#	sys161 kernel "p uw-testbin/pidcheck;q"
	sys161 kernel "p uw-testbin/widefork;q"
#	sys161 kernel "p testbin/forktest;q"
	sys161 kernel "p uw-testbin/hogparty;q"
#	sys161 kernel "p testbin/sty;q"
#	sys161 kernel "p uw-testbin/argtesttest;q"
#	sys161 kernel "p testbin/argtest;q"
#	sys161 kernel "p uw-testbin/argtest;q"
#	sys161 kernel "p testbin/add;q"
elif [ "$1" -eq 1 ]
then
	sys161 kernel "p uw-testbin/vm-data1;q"
	sys161 kernel "p uw-testbin/vm-data3;q"
	sys161 kernel "p uw-testbin/romemwrite;q"
	sys161 kernel "p uw-testbin/vm-crash2;q"

else
	# sys161 -w kernel
	sys161 kernel
fi
