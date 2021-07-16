
#ifndef _GNU_SOURCE
//Need _GNU_SOURCE, sched.h, and unistd.h for setting thread affinity in Linux
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>
#include "laminarFifoClient.h"
#include "laminarFifoServer.h"
#include "timeHelpers.h"
#include "laminarFifoCommon.h"
#include "laminarFifoParams.h"
#include "testParams.h"
#include "laminarFifoRunner.h"
#include "vitisNumaAllocHelpers.h"

void initFIFO(_Atomic int8_t** PartitionCrossingFIFO_readOffsetPtr_re, 
              _Atomic int8_t** PartitionCrossingFIFO_writeOffsetPtr_re, 
              PartitionCrossingFIFO_t** PartitionCrossingFIFO_arrayPtr_re, 
              atomic_flag **serverFlag, atomic_flag **clientFlag, 
              int serverCore, int clientCore){
    *PartitionCrossingFIFO_readOffsetPtr_re = (_Atomic int8_t*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(_Atomic int8_t), clientCore);
    *PartitionCrossingFIFO_writeOffsetPtr_re = (_Atomic int8_t*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(_Atomic int8_t), serverCore);
    *PartitionCrossingFIFO_arrayPtr_re = (PartitionCrossingFIFO_t*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(PartitionCrossingFIFO_t)*(FIFO_LEN_BLKS+1), serverCore); //Alloc an additional block (for empty/full ambiguity resolution)

    *serverFlag = (atomic_flag*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(atomic_flag), serverCore);
    *clientFlag = (atomic_flag*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(atomic_flag), clientCore);

    //Init Flags
    atomic_signal_fence(memory_order_acquire);
    atomic_flag_clear_explicit(*serverFlag, memory_order_release); //Init since this was malloc-ed
    atomic_flag_clear_explicit(*clientFlag, memory_order_release); //Init since this was malloc-ed
    atomic_flag_test_and_set_explicit(*serverFlag, memory_order_acq_rel);
    atomic_flag_test_and_set_explicit(*clientFlag, memory_order_acq_rel);
    
    //Init Ptrs (will not have any initial state in the FIFO)
    atomic_init(*PartitionCrossingFIFO_readOffsetPtr_re, 0);
    if(!atomic_is_lock_free(*PartitionCrossingFIFO_readOffsetPtr_re)){
        printf("Warning: An atomic FIFO offset (PartitionCrossingFIFO_readOffsetPtr_re) was expected to be lock free but is not\n");
    }
    atomic_init(*PartitionCrossingFIFO_writeOffsetPtr_re, 1);
    if(!atomic_is_lock_free(*PartitionCrossingFIFO_writeOffsetPtr_re)){
        printf("Warning: An atomic FIFO offset (PartitionCrossingFIFO_writeOffsetPtr_re) was expected to be lock free but is not\n");
    }

    //Init array
    for(int i = 0; i<FIFO_LEN_BLKS+1; i++){
        for(int j = 0; j<FIFO_BLK_SIZE_CPLX_FLOAT; j++){
            (*PartitionCrossingFIFO_arrayPtr_re)[i].port0_real[j]=0;
        }
        for(int j = 0; j<FIFO_BLK_SIZE_CPLX_FLOAT; j++){
            (*PartitionCrossingFIFO_arrayPtr_re)[i].port0_imag[j]=0;
        }
    }
}

void cleanupFIFO(_Atomic int8_t* PartitionCrossingFIFO_readOffsetPtr_re, 
                 _Atomic int8_t* PartitionCrossingFIFO_writeOffsetPtr_re, 
                 PartitionCrossingFIFO_t* PartitionCrossingFIFO_arrayPtr_re, 
                 atomic_flag *serverFlag, atomic_flag *clientFlag){
    free(PartitionCrossingFIFO_readOffsetPtr_re);
    free(PartitionCrossingFIFO_writeOffsetPtr_re);
    free(PartitionCrossingFIFO_arrayPtr_re);
    free(serverFlag);
    free(clientFlag);
}

fifo_runner_thread_vars_container_t* startThread(_Atomic int8_t* PartitionCrossingFIFO_readOffsetPtr_re, 
                                                 _Atomic int8_t* PartitionCrossingFIFO_writeOffsetPtr_re, 
                                                 PartitionCrossingFIFO_t* PartitionCrossingFIFO_arrayPtr_re,
                                                 _Atomic bool* startTrigger,
                                                 atomic_flag *serverReadyFlag,
                                                 atomic_flag *clientReadyFlag,
                                                 int serverCore, int clientCore){
    //Allocate: fifo_runner_thread_vars_t
    fifo_runner_thread_vars_t *serverThreadVars = (fifo_runner_thread_vars_t*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(fifo_runner_thread_vars_t), serverCore);
    fifo_runner_thread_vars_t *clientThreadVars = (fifo_runner_thread_vars_t*) vitis_aligned_alloc_core(VITIS_MEM_ALIGNMENT, sizeof(fifo_runner_thread_vars_t), clientCore);

    //Set arguments
    //Don't need to align this as it is only used by the master thread which is not actually performing the benchmarking
    fifo_runner_thread_vars_container_t *threadVarContainer = (fifo_runner_thread_vars_container_t*) malloc(sizeof(fifo_runner_thread_vars_container_t));
    threadVarContainer->serverVars = serverThreadVars;
    threadVarContainer->clientVars = clientThreadVars;

    int status;
    status = pthread_attr_init(&(serverThreadVars->attr));
    if (status != 0)
    {
        printf("Could not create server pthread attributes ... exiting");
        exit(1);
    }
    status = pthread_attr_init(&(clientThreadVars->attr));
    if (status != 0)
    {
        printf("Could not create client pthread attributes ... exiting");
        exit(1);
    }

    //Set partition to run with SCHED_FIFO RT Scheduler with Max Priority
    //NOTE! This can lock up the computer if this thread is run on a CPU where system tasks are running.
    status = pthread_attr_setinheritsched(&(serverThreadVars->attr), PTHREAD_EXPLICIT_SCHED);
    if (status != 0)
    {
        printf("Could not set server pthread explicit schedule attribute ... exiting\n");
        exit(1);
    }
    status = pthread_attr_setinheritsched(&(clientThreadVars->attr), PTHREAD_EXPLICIT_SCHED);
    if (status != 0)
    {
        printf("Could not set client pthread explicit schedule attribute ... exiting\n");
        exit(1);
    }

    status = pthread_attr_setschedpolicy(&(serverThreadVars->attr), SCHED_FIFO);
    if (status != 0)
    {
        printf("Could not set server pthread schedule policy to SCHED_FIFO ... exiting\n");
        exit(1);
    }
    status = pthread_attr_setschedpolicy(&(clientThreadVars->attr), SCHED_FIFO);
    if (status != 0)
    {
        printf("Could not set client pthread schedule policy to SCHED_FIFO ... exiting\n");
        exit(1);
    }

    serverThreadVars->threadParams.sched_priority = sched_get_priority_max(SCHED_FIFO);
    status = pthread_attr_setschedparam(&(serverThreadVars->attr), &(serverThreadVars->threadParams));
    if (status != 0)
    {
        printf("Could not set server pthread schedule parameter ... exiting\n");
        exit(1);
    }
    clientThreadVars->threadParams.sched_priority = sched_get_priority_max(SCHED_FIFO);
    status = pthread_attr_setschedparam(&(clientThreadVars->attr), &(clientThreadVars->threadParams));
    if (status != 0)
    {
        printf("Could not set client pthread schedule parameter ... exiting\n");
        exit(1);
    }

    //Set server partition to run on specified CPU
    CPU_ZERO(&(serverThreadVars->cpuset));                                                                           //Clear cpuset
    CPU_SET(serverCore, &(serverThreadVars->cpuset));                                                                //Add CPU to cpuset
    status = pthread_attr_setaffinity_np(&(serverThreadVars->attr), sizeof(cpu_set_t), &(serverThreadVars->cpuset)); //Set thread CPU affinity
    if (status != 0)
    {
        printf("Could not set server thread core affinity ... exiting");
        exit(1);
    }

    //Set client partition to run on specified CPU
    CPU_ZERO(&(clientThreadVars->cpuset));                                                                           //Clear cpuset
    CPU_SET(clientCore, &(clientThreadVars->cpuset));                                                                //Add CPU to cpuset
    status = pthread_attr_setaffinity_np(&(clientThreadVars->attr), sizeof(cpu_set_t), &(clientThreadVars->cpuset)); //Set thread CPU affinity
    if (status != 0)
    {
        printf("Could not set client thread core affinity ... exiting");
        exit(1);
    }

    //Set server arguments
    serverThreadVars->args.PartitionCrossingFIFO_readOffsetPtr_re = PartitionCrossingFIFO_readOffsetPtr_re;
    serverThreadVars->args.PartitionCrossingFIFO_writeOffsetPtr_re = PartitionCrossingFIFO_writeOffsetPtr_re;
    serverThreadVars->args.PartitionCrossingFIFO_arrayPtr_re = PartitionCrossingFIFO_arrayPtr_re;
    serverThreadVars->args.startTrigger = startTrigger;
    serverThreadVars->args.readyFlag = serverReadyFlag;

    //Set client arguments
    clientThreadVars->args.PartitionCrossingFIFO_readOffsetPtr_re = PartitionCrossingFIFO_readOffsetPtr_re;
    clientThreadVars->args.PartitionCrossingFIFO_writeOffsetPtr_re = PartitionCrossingFIFO_writeOffsetPtr_re;
    clientThreadVars->args.PartitionCrossingFIFO_arrayPtr_re = PartitionCrossingFIFO_arrayPtr_re;
    clientThreadVars->args.startTrigger = startTrigger;
    clientThreadVars->args.readyFlag = clientReadyFlag;

    //Start threads
    status = pthread_create(&(serverThreadVars->thread), &(serverThreadVars->attr), fifo_server_thread, &(serverThreadVars->args));
    if (status != 0)
    {
        printf("Could not create a server thread ... exiting");
        errno = status;
        perror(NULL);
        exit(1);
    }
    status = pthread_create(&(clientThreadVars->thread), &(clientThreadVars->attr), fifo_client_thread, &(clientThreadVars->args));
    if (status != 0)
    {
        printf("Could not create a client thread ... exiting");
        errno = status;
        perror(NULL);
        exit(1);
    }

    return threadVarContainer;
}

void collectResults(fifo_runner_thread_vars_container_t **threadVars, double *serverTimes, double *clientTimes, int numFIFOs){
    for(int i = 0; i<numFIFOs; i++){
        int status;
        void *serverResult;
        status = pthread_join(threadVars[i]->serverVars->thread, &serverResult);
        if (status != 0)
        {
            printf("Could not join a server thread ... exiting");
            errno = status;
            perror(NULL);
            exit(1);
        }
        double *serverResultCast = (double*) serverResult;
        serverTimes[i] = *serverResultCast;
        free(serverResult);

        void *clientResult;
        status = pthread_join(threadVars[i]->clientVars->thread, &clientResult);
        if (status != 0)
        {
            printf("Could not join a client thread ... exiting");
            errno = status;
            perror(NULL);
            exit(1);
        }
        double *clientResultCast = (double*) clientResult;
        clientTimes[i] = *clientResultCast;
        free(clientResult);
    }
}

void cleanupThreadVars(fifo_runner_thread_vars_container_t* vars){
    free(vars->serverVars);
    free(vars->clientVars);
    free(vars);
}

void writeResults(int *serverCPUs, int *clientCPUs, double *serverTimes, double *clientTimes, int numFIFOs, char* reportFilename){
    FILE *resultsFile = fopen(reportFilename, "w");
    fprintf(resultsFile, "ServerCPU,ClientCPU,ServerTime,ClientTime,BytesTx,BytesRx\n");

    long long int bytesSent = TRANSACTIONS_BLKS*BLK_SIZE_BYTES;
    for(int i = 0; i<numFIFOs; i++){
        fprintf(resultsFile, "%d,%d,%e,%e,%lld,%lld\n", serverCPUs[i], clientCPUs[i], serverTimes[i], clientTimes[i], bytesSent, bytesSent);
    }

    fclose(resultsFile);
}

/**
 * @param serverCPUs a list of CPUs to serve as the server side of FIFOs.  Each server CPU is pared with a client CPU in clientCPUs
 * @param clientCPUs a list of CPUs to serve as the client side of FIFOs.  Each server CPU is pared with a server CPU in serverCPUs
 * @param numFIFOs the number of FIFOs (also the size of serverCPUs and clientCPUs)
 * @param reportFilename
 */
void runLaminarFifoBench(int *serverCPUs, int *clientCPUs, int numFIFOs, char* reportFilename){
    //Create FIFOs (will allocate write ptr and array on server side)
    _Atomic int8_t* PartitionCrossingFIFO_readOffsetPtr_re[numFIFOs];
    _Atomic int8_t* PartitionCrossingFIFO_writeOffsetPtr_re[numFIFOs];
    PartitionCrossingFIFO_t* PartitionCrossingFIFO_arrayPtr_re[numFIFOs];
    atomic_flag *serverReadyFlag[numFIFOs];
    atomic_flag *clientReadyFlag[numFIFOs];

    for(int i = 0; i<numFIFOs; i++){
        initFIFO(PartitionCrossingFIFO_readOffsetPtr_re+i, PartitionCrossingFIFO_writeOffsetPtr_re+i, PartitionCrossingFIFO_arrayPtr_re+i, serverReadyFlag+i, clientReadyFlag+i, serverCPUs[i], clientCPUs[i]);
    }

    //Create starting trigger
    _Atomic bool* startTrigger = (_Atomic bool*) vitis_aligned_alloc(VITIS_MEM_ALIGNMENT, sizeof(_Atomic bool));
    atomic_signal_fence(memory_order_acquire);
    atomic_store_explicit(startTrigger, false, memory_order_release);

    //Start Threads
    fifo_runner_thread_vars_container_t *threadVars[numFIFOs];
    for(int i = 0; i<numFIFOs; i++){
        threadVars[i] = startThread(PartitionCrossingFIFO_readOffsetPtr_re[i], 
                                    PartitionCrossingFIFO_writeOffsetPtr_re[i], 
                                    PartitionCrossingFIFO_arrayPtr_re[i],
                                    startTrigger,
                                    serverReadyFlag[i],
                                    clientReadyFlag[i],
                                    serverCPUs[i], clientCPUs[i]);
    }

    //Wait for all threads ready
    for(int i = 0; i<numFIFOs; i++){
        bool wait = true;
        while(wait){
            wait = atomic_flag_test_and_set_explicit(serverReadyFlag[i], memory_order_acq_rel);
        }
        wait = true;
        while(wait){
            wait = atomic_flag_test_and_set_explicit(clientReadyFlag[i], memory_order_acq_rel);
        }
    }

    //Start FIFO transfers
    atomic_signal_fence(memory_order_acquire);
    atomic_store_explicit(startTrigger, true, memory_order_release);

    //Wait for threads to finish
    double serverTimes[numFIFOs];
    double clientTimes[numFIFOs];
    collectResults(threadVars, serverTimes, clientTimes, numFIFOs);

    //Write results
    writeResults(serverCPUs, clientCPUs, serverTimes, clientTimes, numFIFOs, reportFilename);

    //Cleanup
    for(int i = 0; i<numFIFOs; i++){
        cleanupThreadVars(threadVars[i]);
    }

    for(int i = 0; i<numFIFOs; i++){
        cleanupFIFO(PartitionCrossingFIFO_readOffsetPtr_re[i], 
                    PartitionCrossingFIFO_writeOffsetPtr_re[i], 
                    PartitionCrossingFIFO_arrayPtr_re[i], 
                    serverReadyFlag[i], clientReadyFlag[i]);
    }
}