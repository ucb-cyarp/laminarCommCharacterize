#ifndef _LAMINAR_FIFO_RUNNER_H
#define _LAMINAR_FIFO_RUNNER_H

#ifndef _GNU_SOURCE
//Need _GNU_SOURCE, sched.h, and unistd.h for setting thread affinity in Linux
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <stdint.h>

#include "laminarFifoCommon.h"

typedef struct {
    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param threadParams;
    cpu_set_t cpuset;
    laminar_fifo_threadArgs_t args;
} fifo_runner_thread_vars_t;

typedef struct {
    fifo_runner_thread_vars_t *serverVars;
    fifo_runner_thread_vars_t *clientVars;
} fifo_runner_thread_vars_container_t;

void runLaminarFifoBench(int *serverCPUs, int *clientCPUs, int numFIFOs, char* reportFilename);

#endif