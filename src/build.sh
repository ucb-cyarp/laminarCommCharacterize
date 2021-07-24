#!/bin/bash
set -e

if [[ -z  ${FIFO_BLK_SIZE_CPLX_FLOAT} ]]
    FIFO_BLK_SIZE_CPLX_FLOAT=128
fi

if [[ -z ${TRANSACTIONS_BLKS} ]]
    TRANSACTIONS_BLKS=20000000
fi

if [[ -z ${CC} ]]
    CC=clang
fi

./collectBuildInfo.sh ${CC}

echo "make clean; make CC=${CC} FIFO_BLK_SIZE_CPLX_FLOAT=${FIFO_BLK_SIZE_CPLX_FLOAT} TRANSACTIONS_BLKS=${TRANSACTIONS_BLKS}" > build.log
make clean; make CC=${CC} FIFO_BLK_SIZE_CPLX_FLOAT=${FIFO_BLK_SIZE_CPLX_FLOAT} TRANSACTIONS_BLKS=${TRANSACTIONS_BLKS} >> build.log