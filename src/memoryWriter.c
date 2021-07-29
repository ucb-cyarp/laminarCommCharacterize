#include "laminarFifoCommon.h" //Transacts in the same blocks as the FIFO: PartitionCrossingFIFO_t
#include "memoryCommon.h"
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "timeHelpers.h"
#include "testParams.h"

//Initializes a segment of memory pointed to by writeBuffer before the benchmark starts
//Durring benchmark writes to the same buffer.  This operation is timed
void *memory_writer_thread(void* args){
    memory_threadArgs_t *args_cast = (memory_threadArgs_t *)args;

    //==== Get Arguments ====
    PartitionCrossingFIFO_t *buffer = args_cast->buffer;
    _Atomic bool *startTrigger = args_cast->startTrigger;
    atomic_flag *readyFlag = args_cast->readyFlag;

    //==== Init Write Array ====
    for(int i = 0; i<MEMORY_ARRAY_SIZE_BLKS; i++){
        for(int j = 0; j<FIFO_BLK_SIZE_CPLX_FLOAT; j++){
            buffer[i].port0_real[j] = 0;
        }
        for(int j = 0; j<FIFO_BLK_SIZE_CPLX_FLOAT; j++){
            buffer[i].port0_imag[j] = 0;
        }
    }

    //==== Setup Temporary for write  ====
    PartitionCrossingFIFO_t writeTmp;
    for(int j = 0; j<FIFO_BLK_SIZE_CPLX_FLOAT; j++){
        writeTmp.port0_real[j] = 0;
    }
    for(int j = 0; j<FIFO_BLK_SIZE_CPLX_FLOAT; j++){
        writeTmp.port0_imag[j] = 0;
    }

    //==== Set initial read location =====
    int bufferIdx = 0;

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

        //Since this is not a FIFO transfer, there is no need for checking pointers or for atomic read/writes with aquire/release ordering

        //Write output array
        //
        {  //Begin Scope for Write
            //Write to array
            __builtin_memcpy_inline(buffer + bufferIdx, &writeTmp, sizeof(PartitionCrossingFIFO_t));
        } //End Scope for Write

        //Increment the ptr position or wrap
        bufferIdx = bufferIdx<(MEMORY_ARRAY_SIZE_BLKS-1) ? bufferIdx+1 : 0;

        //Need to make sure that the memory copy is not optimized out if the content is not checked
        asm volatile(""
        :
        : 
        : "memory");
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

