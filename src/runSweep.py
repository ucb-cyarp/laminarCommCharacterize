#!/usr/bin/env python3

## Runs a sweep of different block sizes without re-booting the system beween runs.

import os
import subprocess
import typing
import argparse
import platform
import datetime
import math

from slackUtils import *

#TODO: Make params?.  Would need to make params in outer script which loads the slack URL
TARGET_BYTES: int = 1000000000
UNIT_SIZE: int = 4*2 #Complex floats (8 bytes) are the unit described in the FIFO structure
BLK_SIZE_START: int = 1 #These are in UNIT_SIZE
BLK_SIZE_END: int = 513 #These are in UNIT_SIZE
BLK_SIZE_STEP: int = 1 #These are in UNIT_SIZE

def main():
    #Parse CLI Arguments for Config File Location
    parser = argparse.ArgumentParser(description='Runs a sweep of the Laminar Comm Characterize test')
    parser.add_argument('--name', type=str, required=True, help='The directory in which the resuts of Laminar Comm Characterize will be located')
    args = parser.parse_args()
    name = args.name

    hostname = platform.node()

    if os.path.exists(name):
        raise ValueError(f'File/Directory {name} already exists')

    os.mkdir(name) #Make the directory into which results should be placed

    blkSizes = range(BLK_SIZE_START, BLK_SIZE_END, BLK_SIZE_STEP)

    for blkSize in blkSizes:
        cur_time = datetime.datetime.now()
        blkSizeBytes = blkSize*UNIT_SIZE
        blockTransactions = math.ceil(TARGET_BYTES/float(blkSizeBytes))
        bytesSent = blockTransactions*blkSizeBytes

        rptDir = os.path.join(name, f'blkSizeBytes{blkSizeBytes:d}')
        rptPrefix = os.path.join(rptDir, 'report') #'report' is the prefix of the report filenames.  It can be changed

        slackStatusPost(f'*Laminar FIFO Characterize Starting*\nBlock Size: {blkSizeBytes:d}\nBlock Transactions: {blockTransactions:d}\nBytes Sent: {bytesSent:d}\nHost: {hostname}\nTime: {cur_time}')

        #Build new version
        cmd = f'FIFO_BLK_SIZE_CPLX_FLOAT={blkSize:d} TRANSACTIONS_BLKS={blockTransactions:d} ./build.sh'
        print('\nRunning: {}\n'.format(cmd))
        rtn = subprocess.call(cmd, shell=True, executable='/bin/bash')
        if rtn != 0:
            slackStatusPost(f'*Laminar FIFO Characterize Failed:x:*\nCMD: {cmd}\nRtnCode: {rtn}\nHost: {hostname}\nTime: {cur_time}')
            raise RuntimeError(f'Laminar FIFO Characterize Failed - CMD: {cmd} RtnCode: {rtn}')

        #Run the test
        os.mkdir(rptDir)
        cmd = f'./commCharaterize {rptPrefix}'
        print('\nRunning: {}\n'.format(cmd))
        rtn = subprocess.call(cmd, shell=True, executable='/bin/bash')
        if rtn != 0:
            slackStatusPost(f'*Laminar FIFO Characterize Failed:x:*\nCMD: {cmd}\nRtnCode: {rtn}\nHost: {hostname}\nTime: {cur_time}')
            raise RuntimeError(f'Laminar FIFO Characterize Failed - CMD: {cmd} RtnCode: {rtn}')

        #Collect the build artifacts
        cmd = f'./collectResults.sh {rptDir}'
        print('\nRunning: {}\n'.format(cmd))
        rtn = subprocess.call(cmd, shell=True, executable='/bin/bash')
        if rtn != 0:
            slackStatusPost(f'*Laminar FIFO Characterize Failed:x:*\nCMD: {cmd}\nRtnCode: {rtn}\nHost: {hostname}\nTime: {cur_time}')
            raise RuntimeError(f'Laminar FIFO Characterize Failed - CMD: {cmd} RtnCode: {rtn}')

    slackStatusPost('*Laminar FIFO Characterize Finishing :white_check_mark:*\nHost: ' + hostname + '\n' + 'Time: ' + str(cur_time))

if __name__ == "__main__":
    main()