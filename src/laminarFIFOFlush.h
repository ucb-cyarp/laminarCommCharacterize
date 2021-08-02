#ifndef _LAMINAR_FIFO_FLUSH_H
#define _LAMINAR_FIFO_FLUSH_H

#include <memory.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <stdint.h>

#define CACHE_LINE_WIDTH (64)

//While write ordered WRT the same cache lines, the AMD manual states that CLFLUSH is not ordered relative to write operations to other cache lines
//It also appears there would be no ordering guarentee that CLFLUSH would execute after we are finished reading the array to copy, possibly resulting in the line being evicted then needing to be pulled back
//As a result, we will put MFENCE instrucitons arround CLFLUSH to ensure that the read is finished before flushing and that the flush is finished before updating the write ptr
//See AMD General-Purpose Instruction Reference "CLFLUSH".  Pg 144 for 24594—Rev. 3.32—March 2021
#define cacheInvalidateRange(ptr_uncast, range){\
    {\
        char* ptr = (char*) (ptr_uncast);\
\
        size_t numCacheLines = (range)/CACHE_LINE_WIDTH;\
        _mm_mfence();\
\
        for(size_t i = 0; i<numCacheLines; i++){\
            _mm_clflushopt(ptr+i*CACHE_LINE_WIDTH);\
        }\
\
\
        if(((uintptr_t) ptr) % CACHE_LINE_WIDTH != 0){\
             _mm_clflushopt(ptr+range-1);\
        }\
\
        _mm_mfence();\
    }\
\
}

#endif