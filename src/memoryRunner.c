#ifndef _GNU_SOURCE
//Need _GNU_SOURCE, sched.h, and unistd.h for setting thread affinity in Linux
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include "memoryCommon.h"
#include "memoryRunner.h"
#include "testParams.h"
#include "memoryReader.h"
#include "errno.h"
#include "vitisNumaAllocHelpers.h"

void initMemoryBuffer(PartitionCrossingFIFO_t** buffer_arrayPtr_re, 
                atomic_flag **readyFlag, 
                int core){
    *buffer_arrayPtr_re = (PartitionCrossingFIFO_t*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, MEMORY_ARRAY_SIZE_BYTES, core);

    *readyFlag = (atomic_flag*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(atomic_flag), core);

    //Init Flags
    atomic_signal_fence(memory_order_acquire);
    atomic_flag_clear_explicit(*readyFlag, memory_order_release); //Init since this was malloc-ed
    atomic_flag_test_and_set_explicit(*readyFlag, memory_order_acq_rel);

    //Init array
    for(int i = 0; i<FIFO_LEN_BLKS+1; i++){
        for(int j = 0; j<FIFO_BLK_SIZE_CPLX_FLOAT; j++){
            (*buffer_arrayPtr_re)[i].port0_real[j]=0;
        }
        for(int j = 0; j<FIFO_BLK_SIZE_CPLX_FLOAT; j++){
            (*buffer_arrayPtr_re)[i].port0_imag[j]=0;
        }
    }
}

void cleanupMemoryBuffer(PartitionCrossingFIFO_t* buffer_arrayPtr_re, atomic_flag *readyFlag){
    free(buffer_arrayPtr_re);
    free(readyFlag);
}

memory_runner_thread_vars_t* startMemoryThread(PartitionCrossingFIFO_t* buffer_arrayPtr_re,
                                               _Atomic bool* startTrigger,
                                               atomic_flag *readyFlag,
                                               int core,
                                               void* (*memory_thread_fun)(void*)){
    //Allocate: fifo_runner_thread_vars_t
    memory_runner_thread_vars_t *readerThreadVars = (memory_runner_thread_vars_t*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(memory_runner_thread_vars_t), core);

    int status;
    status = pthread_attr_init(&(readerThreadVars->attr));
    if (status != 0)
    {
        printf("Could not create server pthread attributes ... exiting");
        exit(1);
    }

    //Set partition to run with SCHED_FIFO RT Scheduler with Max Priority
    //NOTE! This can lock up the computer if this thread is run on a CPU where system tasks are running.
    status = pthread_attr_setinheritsched(&(readerThreadVars->attr), PTHREAD_EXPLICIT_SCHED);
    if (status != 0)
    {
        printf("Could not set server pthread explicit schedule attribute ... exiting\n");
        exit(1);
    }

    status = pthread_attr_setschedpolicy(&(readerThreadVars->attr), SCHED_FIFO);
    if (status != 0)
    {
        printf("Could not set server pthread schedule policy to SCHED_FIFO ... exiting\n");
        exit(1);
    }

    readerThreadVars->threadParams.sched_priority = sched_get_priority_max(SCHED_FIFO);
    status = pthread_attr_setschedparam(&(readerThreadVars->attr), &(readerThreadVars->threadParams));
    if (status != 0)
    {
        printf("Could not set server pthread schedule parameter ... exiting\n");
        exit(1);
    }

    //Set server partition to run on specified CPU
    CPU_ZERO(&(readerThreadVars->cpuset));                                                                           //Clear cpuset
    CPU_SET(core, &(readerThreadVars->cpuset));                                                          //Add CPU to cpuset
    status = pthread_attr_setaffinity_np(&(readerThreadVars->attr), sizeof(cpu_set_t), &(readerThreadVars->cpuset)); //Set thread CPU affinity
    if (status != 0)
    {
        printf("Could not set server thread core affinity ... exiting");
        exit(1);
    }

    //Set server arguments
    readerThreadVars->args.buffer = buffer_arrayPtr_re;
    readerThreadVars->args.startTrigger = startTrigger;
    readerThreadVars->args.readyFlag = readyFlag;

    //Start threads
    status = pthread_create(&(readerThreadVars->thread), &(readerThreadVars->attr), memory_thread_fun, &(readerThreadVars->args));
    if (status != 0)
    {
        printf("Could not create a server thread ... exiting");
        errno = status;
        perror(NULL);
        exit(1);
    }

    return readerThreadVars;
}

memory_runner_thread_vars_t* startMemoryReaderThread(PartitionCrossingFIFO_t* buffer_arrayPtr_re,
                                               _Atomic bool* startTrigger,
                                               atomic_flag *readyFlag,
                                               int core){
    return startMemoryThread(buffer_arrayPtr_re, startTrigger, readyFlag, core, memory_reader_thread);
}

void collectResultsMemory(memory_runner_thread_vars_t **threadVars, double *memoryTimes, int numFIFOs){
    for(int i = 0; i<numFIFOs; i++){
        int status;
        void *memoryResult;
        status = pthread_join(threadVars[i]->thread, &memoryResult);
        if (status != 0)
        {
            printf("Could not join a server thread ... exiting");
            errno = status;
            perror(NULL);
            exit(1);
        }
        double *memoryResultCast = (double*) memoryResult;
        memoryTimes[i] = *memoryResultCast;
        free(memoryResultCast);
    }
}

void cleanupMemoryThreadVars(memory_runner_thread_vars_t* vars){
    free(vars);
}

void writeMemoryResults(int *cpus, double *memoryTimes, int numFIFOs, char* reportFilename){
    FILE *resultsFile = fopen(reportFilename, "w");
    fprintf(resultsFile, "CPU,MemoryTime,BytesTransacted,MemArrayBytes\n");

    long long int bytesTransacted = TRANSACTIONS_BLKS*BLK_SIZE_BYTES;
    long long int memArrayBytes = MEMORY_ARRAY_SIZE_BYTES;
    for(int i = 0; i<numFIFOs; i++){
        fprintf(resultsFile, "%d,%e,%lld,%lld\n", cpus[i], memoryTimes[i], bytesTransacted, memArrayBytes);
    }

    fclose(resultsFile);
}

/**
 * @param serverCPUs a list of CPUs to run the memory test
 * @param numFIFOs the number of FIFOs (also the size of serverCPUs and clientCPUs)
 * @param reportFilename
 */
void runMemoryBench(int *cpus, int numFIFOs, char* reportFilename, void* (*memory_thread_fun)(void*)){
    //Create buffers
    PartitionCrossingFIFO_t* buffers[numFIFOs];
    atomic_flag *readyFlags[numFIFOs];

    for(int i = 0; i<numFIFOs; i++){
        initMemoryBuffer(buffers+i, readyFlags+i, cpus[i]);
    }

    //Create starting trigger
    _Atomic bool* startTrigger = (_Atomic bool*) vitis_aligned_alloc(VITIS_MEM_ALIGNMENT, sizeof(_Atomic bool));
    atomic_signal_fence(memory_order_acquire);
    atomic_store_explicit(startTrigger, false, memory_order_release);

    //Start Threads
    memory_runner_thread_vars_t *threadVars[numFIFOs];
    for(int i = 0; i<numFIFOs; i++){
        threadVars[i] = startMemoryThread(buffers[i], startTrigger, readyFlags[i], cpus[i], memory_thread_fun);
    }

    //Wait for all threads ready
    for(int i = 0; i<numFIFOs; i++){
        bool wait = true;
        while(wait){
            wait = atomic_flag_test_and_set_explicit(readyFlags[i], memory_order_acq_rel);
        }
    }

    //Start FIFO transfers
    atomic_signal_fence(memory_order_acquire);
    atomic_store_explicit(startTrigger, true, memory_order_release);

    //Wait for threads to finish
    double memoryTimes[numFIFOs];
    collectResultsMemory(threadVars, memoryTimes, numFIFOs);

    //Write results
    writeMemoryResults(cpus, memoryTimes, numFIFOs, reportFilename);

    //Cleanup
    for(int i = 0; i<numFIFOs; i++){
        cleanupMemoryThreadVars(threadVars[i]);
    }

    for(int i = 0; i<numFIFOs; i++){
        cleanupMemoryBuffer(buffers[i], readyFlags[i]);
    }
}