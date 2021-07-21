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
    double* dstDouble = (double*) dst;\
    double* srcDouble = (double*) src;\
    for(int i = 0; i<avx256Copies; i++){\
        __m256d ldVal =  _mm256_loadu_pd(srcDouble);\
        _mm256_storeu_pd (dstDouble, ldVal);\
        srcDouble += 4;\
        dstDouble += 4;\
    }\
    for(int i = 0; i<avx128Copies; i++){\
        __m128d ldVal =  _mm_loadu_pd(srcDouble);\
        _mm_storeu_pd (dstDouble, ldVal);\
        srcDouble += 2;\
        dstDouble += 2;\
    }\
    for(int i = 0; i<fp64Copies; i++){\
        double ldVal = *srcDouble;\
        *dstDouble = ldVal;\
        srcDouble++;\
        dstDouble++;\
    }\
    char* dst = (char*) dstDouble;\
    char* src = (char*) srcDouble;\
    for(int i = 0; i<bytes; i++){\
        char ldVal = *src;\
        *dst = ldVal;\
        src++;\
        dst++;\
    }\
}\
}

#endif