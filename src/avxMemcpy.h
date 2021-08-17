#ifndef _AVX_MEMCPY_H
#define _AVX_MEMCPY_H

#include <immintrin.h>
#include <assert.h>

#define avxMemcpy(DST, SRC, BYTES){\
{\
    int bytes = BYTES;\
    int avx256Copies = bytes/32;\
    bytes -= avx256Copies*32;\
    int avx128Copies = bytes/16;\
    bytes -= avx128Copies*16;\
    int fp64Copies = bytes/8;\
    bytes -= fp64Copies*8;\
    \
    char* dst = (char*) (DST);\
    char* src = (char*) (SRC);\
    \
    double* dstDouble = (double*) dst;\
    double* srcDouble = (double*) src;\
    for(int i = 0; i<avx256Copies; i++){\
        __m256d ldVal =  _mm256_loadu_pd(srcDouble+i*4);\
        _mm256_storeu_pd (dstDouble+i*4, ldVal);\
    }\
    for(int i = 0; i<avx128Copies; i++){\
        __m128d ldVal =  _mm_loadu_pd(srcDouble+avx256Copies*4+i*2);\
        _mm_storeu_pd (dstDouble+avx256Copies*4+i*2, ldVal);\
    }\
    for(int i = 0; i<fp64Copies; i++){\
        double ldVal = srcDouble[avx256Copies*4+avx128Copies*2+i];\
        dstDouble[avx256Copies*4+avx128Copies*2+i] = ldVal;\
    }\
    for(int i = 0; i<bytes; i++){\
        char ldVal = src[(avx256Copies*4+avx128Copies*2+fp64Copies)*8+i];\
        dst[(avx256Copies*4+avx128Copies*2+fp64Copies)*8+i] = ldVal;\
    }\
}\
}

//This version relies on the src and dst arrays to be aligned to 256 bit boundaries
//It also relies on the BYTES being a multiple of the 256 bit alignment - only does AVX2 load/stores
//Can be updated to allow smaller BYTES - using SSE nontemporal intrinsics and quadword intrinsics
//Can also be re-written to do in-place re-aligment
#define avxNonTemporalLoadMemcpyAligned(DST, SRC, BYTES){\
{\
    static_assert((BYTES) % 32 == 0, "BYTES must be a multiple of 32");\
\
    int bytes = (BYTES);\
    int avx256Copies = bytes/32;\
    \
    __m256i* dstDouble = (__m256i*) (DST);\
    __m256i* srcDouble = (__m256i*) (SRC);\
    for(int i = 0; i<avx256Copies; i++){\
        __m256i ldVal = _mm256_stream_load_si256(srcDouble+i);\
        _mm256_store_si256(dstDouble+i, ldVal);\
    }\
}\
}

#define avxNonTemporalStoreMemcpyAligned(DST, SRC, BYTES){\
{\
    static_assert((BYTES) % 32 == 0, "BYTES must be a multiple of 32");\
\
    int bytes = (BYTES);\
    int avx256Copies = bytes/32;\
    \
    __m256i* dstDouble = (__m256i*) (DST);\
    __m256i* srcDouble = (__m256i*) (SRC);\
    for(int i = 0; i<avx256Copies; i++){\
        __m256i ldVal = _mm256_load_si256(srcDouble+i);\
        _mm256_stream_si256(dstDouble+i, ldVal);\
    }\
}\
}

#endif