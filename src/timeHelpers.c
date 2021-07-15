#ifndef _GNU_SOURCE
#define _GNU_SOURCE //For clock_gettime
#endif

#include "timeHelpers.h"

double difftimespec(timespec_t* a, timespec_t* b){
    return (a->tv_sec - b->tv_sec) + ((double) (a->tv_nsec - b->tv_nsec))*(0.000000001);
}
double timespecToDouble(timespec_t* a){
    double a_double = a->tv_sec + (a->tv_nsec)*(0.000000001);
    return a_double;
}
