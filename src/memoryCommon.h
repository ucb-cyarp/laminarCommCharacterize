#ifndef _MEMORY_COMMON_H
#define _MEMORY_COMMON_H

#include "laminarFifoCommon.h"

//For Ryzen, there is a 32 KByte L1 Cache, a 512 KByte L2 Cache, and a shared 4 or 16 Mbyte L3 victim cache

#ifndef MEMORY_ARRAY_SIZE_BYTE_TARGET
    #define MEMORY_ARRAY_SIZE_BYTE_TARGET (16512000*4) //NOTE: This should be larger than 2x the cache size so that, after writing, the value being read will not be in the cache.
#endif

#define MEMORY_ARRAY_SIZE_BLKS (MEMORY_ARRAY_SIZE_BYTE_TARGET/sizeof(PartitionCrossingFIFO_t)) //Trunkate
#define MEMORY_ARRAY_SIZE_BYTES (MEMORY_ARRAY_SIZE_BLKS*sizeof(PartitionCrossingFIFO_t))

typedef struct {
    PartitionCrossingFIFO_t *buffer; //Will read/write into this
    _Atomic bool *startTrigger; //This is shared by all threads
    atomic_flag *readyFlag; //This is unique to each thread
} memory_threadArgs_t;

#endif