#ifndef _AVX_MEMCPY_H
#define _AVX_MEMCPY_H

#include <immintrin.h>

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
        __m256d ldVal =  _mm256_load_pd(srcDouble+i*4);\
        _mm256_store_pd (dstDouble+i*4, ldVal);\
    }\
    for(int i = 0; i<avx128Copies; i++){\
        __m128d ldVal =  _mm_load_pd(srcDouble+avx256Copies*4+i*2);\
        _mm_store_pd (dstDouble+avx256Copies*4+i*2, ldVal);\
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

#endif