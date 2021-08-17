#include "laminarFifoCommon.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "timeHelpers.h"
#include "testParams.h"

#include "avxMemcpy.h"

void *fifo_server_thread(void* args) __attribute__((no_builtin("memcpy"))) { //https://clang.llvm.org/docs/AttributeReference.html#no-builtin
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
    //Make sure this is aligned to 32 bytes (256 bits).  My hope was that PartitionCrossingFIFO_t would be aligned to its size (1024 bytes which would be aligned to 32 bytes too)
    //However, based on printing the address, it looks like this is not nessisarily true
    PartitionCrossingFIFO_t *PartitionCrossingFIFO_writeTmp = aligned_alloc(VITIS_MEM_ALIGNMENT, sizeof(PartitionCrossingFIFO_t));

    //==== Init write temp ====
    for(int i = 0; i<FIFO_BLK_SIZE_CPLX_FLOAT; i++){
        PartitionCrossingFIFO_writeTmp->port0_real[i] = 0;
    }
    for(int i = 0; i<FIFO_BLK_SIZE_CPLX_FLOAT; i++){
        PartitionCrossingFIFO_writeTmp->port0_imag[i] = 0;
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
        : "=rm" (*PartitionCrossingFIFO_writeTmp)
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

        _mm_mfence(); //No loading/storing should cross this line.  It in general should not because speculative writing should not be comitted.  See AMD Archetecture Programmer's Manual: 3.9.1.2 Write Ordering
        
        //Write output FIFO(s)
        //  --- Pulled from generated Laminar code (bool changed from vitisBool_t to bool)
        { //Begin Scope for PartitionCrossingFIFO FIFO Write
            int PartitionCrossingFIFO_writeOffsetPtr_re_local = PartitionCrossingFIFO_writeOffsetCached_re;
            //Write into array
            avxNonTemporalStoreMemcpyAligned(PartitionCrossingFIFO_arrayPtr_re + PartitionCrossingFIFO_writeOffsetPtr_re_local, PartitionCrossingFIFO_writeTmp, sizeof(PartitionCrossingFIFO_t));
            if (PartitionCrossingFIFO_writeOffsetPtr_re_local >= FIFO_LEN_BLKS)
            {
                PartitionCrossingFIFO_writeOffsetPtr_re_local = 0;
            }
            else
            {
                PartitionCrossingFIFO_writeOffsetPtr_re_local++;
            }
            PartitionCrossingFIFO_writeOffsetCached_re = PartitionCrossingFIFO_writeOffsetPtr_re_local;
            _mm_sfence(); //Stores should not cross this line.  Should finish storing the new items in the buffer before 
            //Update Write Ptr
            atomic_store_explicit(PartitionCrossingFIFO_writeOffsetPtr_re, PartitionCrossingFIFO_writeOffsetPtr_re_local, memory_order_release);
        } //End Scope for PartitionCrossingFIFO FIFO Write
    }

    timespec_t stopTime;
    asm volatile("" ::: "memory"); //Stop Re-ordering of timer
    clock_gettime(CLOCK_MONOTONIC, &stopTime);
    asm volatile("" ::: "memory"); //Stop Re-ordering of timer

    free(PartitionCrossingFIFO_writeTmp);

    //Return results
    double* duration = malloc(sizeof(double));
    *duration = difftimespec(&stopTime, &startTime);
    return duration;
}