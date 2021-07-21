#!/bin/bash
FIFO_BLK_SIZE_CPLX_FLOAT=16
TRANSACTIONS_BLKS=20000000

echo "make clean; make CC=clang FIFO_BLK_SIZE_CPLX_FLOAT=${FIFO_BLK_SIZE_CPLX_FLOAT}" > build.log
make clean; make CC=clang FIFO_BLK_SIZE_CPLX_FLOAT=${FIFO_BLK_SIZE_CPLX_FLOAT} TRANSACTIONS_BLKS=${TRANSACTIONS_BLKS} >> build.log
