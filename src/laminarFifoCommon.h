#ifndef _LAMINAR_FIFO_COMMON
#define _LAMINAR_FIFO_COMMON

#include <stdatomic.h>
#include <stdbool.h>
#include "laminarFifoParams.h"

typedef struct {
    float port0_real[FIFO_BLK_SIZE_CPLX_FLOAT];
    float port0_imag[FIFO_BLK_SIZE_CPLX_FLOAT];
} PartitionCrossingFIFO_t;

typedef struct {
    _Atomic int8_t *PartitionCrossingFIFO_readOffsetPtr_re;
    _Atomic int8_t *PartitionCrossingFIFO_writeOffsetPtr_re;
    PartitionCrossingFIFO_t *PartitionCrossingFIFO_arrayPtr_re;
    _Atomic bool *startTrigger; //This is shared by all threads
    atomic_flag *readyFlag; //This is unique to each thread
} laminar_fifo_threadArgs_t;

#define BLK_SIZE_BYTES (sizeof(PartitionCrossingFIFO_t))

#endif