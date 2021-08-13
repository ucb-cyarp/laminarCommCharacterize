#include "laminarFifoCommon.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "timeHelpers.h"
#include "testParams.h"

void *fifo_client_thread(void* args){
    laminar_fifo_threadArgs_t *args_cast = (laminar_fifo_threadArgs_t *)args;

    //==== Get Arguments ====
    _Atomic int8_t *PartitionCrossingFIFO_readOffsetPtr_re = args_cast->PartitionCrossingFIFO_readOffsetPtr_re;
    _Atomic int8_t *PartitionCrossingFIFO_writeOffsetPtr_re = args_cast->PartitionCrossingFIFO_writeOffsetPtr_re;
    PartitionCrossingFIFO_t *PartitionCrossingFIFO_arrayPtr_re = args_cast->PartitionCrossingFIFO_arrayPtr_re;
    _Atomic bool *startTrigger = args_cast->startTrigger;
    atomic_flag *readyFlag = args_cast->readyFlag;

    //==== Setup Input FIFOs ====
    int8_t PartitionCrossingFIFO_writeOffsetCached_re;
    int8_t PartitionCrossingFIFO_readOffsetCached_re;
    PartitionCrossingFIFO_writeOffsetCached_re = atomic_load_explicit(PartitionCrossingFIFO_writeOffsetPtr_re, memory_order_acquire);
    PartitionCrossingFIFO_readOffsetCached_re = atomic_load_explicit(PartitionCrossingFIFO_readOffsetPtr_re, memory_order_acquire);
    PartitionCrossingFIFO_t PartitionCrossingFIFO_N2_TO_1_0_readTmp;

    //==== Setup duty cycle ====
    double readTimeAccum = 0;

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
        //Wait for input FIFO(s) to be ready
        //  --- Pulled from generated Laminar code (bool changed from vitisBool_t to bool)
        bool inputFIFOsReady = false;
        while (!inputFIFOsReady)
        {
            inputFIFOsReady = true;
            {
                bool PartitionCrossingFIFO_notEmpty_re = (!((PartitionCrossingFIFO_writeOffsetCached_re - PartitionCrossingFIFO_readOffsetCached_re == 1) || (PartitionCrossingFIFO_writeOffsetCached_re - PartitionCrossingFIFO_readOffsetCached_re == -FIFO_LEN_BLKS)));
                if (!(PartitionCrossingFIFO_notEmpty_re))
                {
                    PartitionCrossingFIFO_writeOffsetCached_re = atomic_load_explicit(PartitionCrossingFIFO_writeOffsetPtr_re, memory_order_acquire);
                    PartitionCrossingFIFO_notEmpty_re = (!((PartitionCrossingFIFO_writeOffsetCached_re - PartitionCrossingFIFO_readOffsetCached_re == 1) || (PartitionCrossingFIFO_writeOffsetCached_re - PartitionCrossingFIFO_readOffsetCached_re == -FIFO_LEN_BLKS)));
                }
                inputFIFOsReady &= PartitionCrossingFIFO_notEmpty_re;
            }
        }

        timespec_t startReadingTime;
        asm volatile("" ::: "memory"); //Stop Re-ordering of timer
        clock_gettime(CLOCK_MONOTONIC, &startReadingTime);
        asm volatile("" ::: "memory"); //Stop Re-ordering of timer

        //Read input FIFO(s)
        //  --- Pulled from generated Laminar code (bool changed from vitisBool_t to bool)
        {  //Begin Scope for PartitionCrossingFIFO FIFO Read
            int PartitionCrossingFIFO_readOffsetPtr_re_local = PartitionCrossingFIFO_readOffsetCached_re;
            if (PartitionCrossingFIFO_readOffsetPtr_re_local >= FIFO_LEN_BLKS)
            {
                PartitionCrossingFIFO_readOffsetPtr_re_local = 0;
            }
            else
            {
                PartitionCrossingFIFO_readOffsetPtr_re_local++;
            }

            //Read from array
            __builtin_memcpy_inline(&PartitionCrossingFIFO_N2_TO_1_0_readTmp, PartitionCrossingFIFO_arrayPtr_re + PartitionCrossingFIFO_readOffsetPtr_re_local, sizeof(PartitionCrossingFIFO_t));
            PartitionCrossingFIFO_readOffsetCached_re = PartitionCrossingFIFO_readOffsetPtr_re_local;
            //Update Read Ptr
            atomic_store_explicit(PartitionCrossingFIFO_readOffsetPtr_re, PartitionCrossingFIFO_readOffsetPtr_re_local, memory_order_release);
        } //End Scope for PartitionCrossingFIFO_N2_TO_1_0 FIFO Read

        //Need to make sure that the memory copy is not optimized out if the content is not checked
        asm volatile(""
        :
        : "rm" (PartitionCrossingFIFO_N2_TO_1_0_readTmp)
        :);

        timespec_t stopReadingTime;
        asm volatile("" ::: "memory"); //Stop Re-ordering of timer
        clock_gettime(CLOCK_MONOTONIC, &stopReadingTime);
        asm volatile("" ::: "memory"); //Stop Re-ordering of timer
        double readDuration = difftimespec(&stopReadingTime, &startReadingTime);

        readTimeAccum += readDuration;

        double tgtStallDuration = readDuration/(TGT_DUTY_CYCLE) - readDuration;

        //Try to wait for such an amount of time that the target duty cycle is met
        double stallDuration;
        do{
            timespec_t stallTime;
            asm volatile("" ::: "memory"); //Stop Re-ordering of timer
            clock_gettime(CLOCK_MONOTONIC, &stallTime);
            asm volatile("" ::: "memory"); //Stop Re-ordering of timer
            stallDuration = difftimespec(&stallTime, &stopReadingTime);
        }while(stallDuration<tgtStallDuration);
    }

    timespec_t stopTime;
    asm volatile("" ::: "memory"); //Stop Re-ordering of timer
    clock_gettime(CLOCK_MONOTONIC, &stopTime);
    asm volatile("" ::: "memory"); //Stop Re-ordering of timer

    //Return results
    laminar_fifo_result_t* result = malloc(sizeof(laminar_fifo_result_t));
    result->duration = difftimespec(&stopTime, &startTime);
    result->readWriteTime = readTimeAccum;
    return result;
}