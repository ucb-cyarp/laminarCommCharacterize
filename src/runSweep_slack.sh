#!/bin/bash

name=$1

if [[ ${MODULES_CMD} ]]; then
    module load aocc
fi

##Get the secret key for slack
source ./secretkey.sh

##Run the sweep
python3 ./runSweep.py --name $name