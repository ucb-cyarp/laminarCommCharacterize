#!/bin/bash
set -e

if [[ -z  ${FIFO_BLK_SIZE_CPLX_FLOAT} ]]; then
    FIFO_BLK_SIZE_CPLX_FLOAT=128
fi

if [[ -z ${TRANSACTIONS_BLKS} ]]; then
    TRANSACTIONS_BLKS=20000000
fi

if [[ -z ${TGT_DUTY_CYCLE} ]]; then
    TGT_DUTY_CYCLE=1.0
fi

if [[ -z ${FIFO_TESTS} ]]; then
    FIFO_TESTS=1 #Run FIFO Tests by Default
fi

if [[ -z ${MEM_TESTS} ]]; then
    MEM_TESTS=1 #Run MEM Tests by Default
fi

if [[ -z ${CC} ]]; then
    CC=clang
fi

./collectBuildInfo.sh ${CC}

echo "make clean; make CC=${CC} FIFO_BLK_SIZE_CPLX_FLOAT=${FIFO_BLK_SIZE_CPLX_FLOAT} TRANSACTIONS_BLKS=${TRANSACTIONS_BLKS} TGT_DUTY_CYCLE=${TGT_DUTY_CYCLE} FIFO_TESTS=${FIFO_TESTS} MEM_TESTS=${MEM_TESTS}" > build.log
make clean; make CC=${CC} FIFO_BLK_SIZE_CPLX_FLOAT=${FIFO_BLK_SIZE_CPLX_FLOAT} TRANSACTIONS_BLKS=${TRANSACTIONS_BLKS} TGT_DUTY_CYCLE=${TGT_DUTY_CYCLE} FIFO_TESTS=${FIFO_TESTS} MEM_TESTS=${MEM_TESTS} >> build.log