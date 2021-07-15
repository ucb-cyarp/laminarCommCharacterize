#include "laminarFifoCommon.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "timeHelpers.h"
#include "testParams.h"

void *fifo_server_thread(void* args){
    laminar_fifo_threadArgs_t *args_cast = (laminar_fifo_threadArgs_t *)args;

    //==== Get Arguments ====
    _Atomic int8_t *PartitionCrossingFIFO_readOffsetPtr_re = args_cast->PartitionCrossingFIFO_readOffsetPtr_re;
    _Atomic int8_t *PartitionCrossingFIFO_writeOffsetPtr_re = args_cast->PartitionCrossingFIFO_writeOffsetPtr_re;
    PartitionCrossingFIFO_t *PartitionCrossingFIFO_arrayPtr_re = args_cast->PartitionCrossingFIFO_arrayPtr_re;
    _Atomic bool *startTrigger = args_cast->startTrigger;
    atomic_flag *readyFlag = args_cast->readyFlag;

    //==== Setup Output FIFOs ====
    int8_t PartitionCrossingFIFO_writeOffsetCached_re;
    int8_t PartitionCrossingFIFO_readOffsetCached_re;
    PartitionCrossingFIFO_writeOffsetCached_re = atomic_load_explicit(PartitionCrossingFIFO_writeOffsetPtr_re, memory_order_acquire);
    PartitionCrossingFIFO_readOffsetCached_re = atomic_load_explicit(PartitionCrossingFIFO_readOffsetPtr_re, memory_order_acquire);
    PartitionCrossingFIFO_t PartitionCrossingFIFO_writeTmp;

    //==== Init write temp ====
    for(int i = 0; i<FIFO_BLK_SIZE_CPLX_FLOAT; i++){
        PartitionCrossingFIFO_writeTmp.port0_real[i] = 0;
    }
    for(int i = 0; i<FIFO_BLK_SIZE_CPLX_FLOAT; i++){
        PartitionCrossingFIFO_writeTmp.port0_imag[i] = 0;
    }

    //==== Signal Ready ====
    atomic_thread_fence(memory_order_acquire);
    atomic_flag_clear_explicit(readyFlag, memory_order_release);

    //==== Wait for trigger ====
    bool go = false;
    while (!go){
        go = atomic_load_explicit(startTrigger, memory_order_acquire);
        atomic_thread_fence(memory_order_release);
    }

    //==== Start Test ====
    //Start timer
    timespec_t startTime;
    asm volatile("" ::: "memory"); //Stop Re-ordering of timer
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    asm volatile("" ::: "memory"); //Stop Re-ordering of timer

    //Run for specified number of itterations
    for(int64_t blksTransfered = 0; blksTransfered<TRANSACTIONS_BLKS; blksTransfered++){

        //Try to make sure the copy is not optimized out by signalling to compiler that the write tmp is modified.  It is not actually modified
        asm volatile(""
        : "=rm" (PartitionCrossingFIFO_writeTmp)
        : 
        :);

        //Wait for output FIFOs to be ready
        //  --- Pulled from generated Laminar code (bool changed from vitisBool_t to bool)
        bool outputFIFOsReady = false;
        while (!outputFIFOsReady)
        {
            outputFIFOsReady = true;
            {
                bool PartitionCrossingFIFO_notFull_re = (PartitionCrossingFIFO_readOffsetCached_re != PartitionCrossingFIFO_writeOffsetCached_re);
                if (!(PartitionCrossingFIFO_notFull_re))
                {
                    PartitionCrossingFIFO_readOffsetCached_re = atomic_load_explicit(PartitionCrossingFIFO_readOffsetPtr_re, memory_order_acquire);
                    PartitionCrossingFIFO_notFull_re = (PartitionCrossingFIFO_readOffsetCached_re != PartitionCrossingFIFO_writeOffsetCached_re);
                }
                outputFIFOsReady &= PartitionCrossingFIFO_notFull_re;
            }
        }

        //Write output FIFO(s)
        //  --- Pulled from generated Laminar code (bool changed from vitisBool_t to bool)
        { //Begin Scope for PartitionCrossingFIFO FIFO Write
            int PartitionCrossingFIFO_writeOffsetPtr_re_local = PartitionCrossingFIFO_writeOffsetCached_re;
            //Write into array
            __builtin_memcpy_inline(PartitionCrossingFIFO_arrayPtr_re + PartitionCrossingFIFO_writeOffsetPtr_re_local, &PartitionCrossingFIFO_writeTmp, sizeof(PartitionCrossingFIFO_t));
            if (PartitionCrossingFIFO_writeOffsetPtr_re_local >= FIFO_LEN_BLKS)
            {
                PartitionCrossingFIFO_writeOffsetPtr_re_local = 0;
            }
            else
            {
                PartitionCrossingFIFO_writeOffsetPtr_re_local++;
            }
            PartitionCrossingFIFO_writeOffsetCached_re = PartitionCrossingFIFO_writeOffsetPtr_re_local;
            //Update Write Ptr
            atomic_store_explicit(PartitionCrossingFIFO_writeOffsetPtr_re, PartitionCrossingFIFO_writeOffsetPtr_re_local, memory_order_release);
        } //End Scope for PartitionCrossingFIFO FIFO Write
    }

    timespec_t stopTime;
    asm volatile("" ::: "memory"); //Stop Re-ordering of timer
    clock_gettime(CLOCK_MONOTONIC, &stopTime);
    asm volatile("" ::: "memory"); //Stop Re-ordering of timer

    //Return results
    double* duration = malloc(sizeof(double));
    *duration = difftimespec(&stopTime, &startTime);
    return duration;
}