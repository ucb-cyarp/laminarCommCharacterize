#!/bin/bash

name=$1

##Get the secret key for slack
source ./secretkey.sh

##Run the sweep
python3 ./runSweep.py --name $name