#ifndef _MEMORY_RUNNER_H
#define _MEMORY_RUNNER_H

#ifndef _GNU_SOURCE
//Need _GNU_SOURCE, sched.h, and unistd.h for setting thread affinity in Linux
#define _GNU_SOURCE
#endif

#include <unistd.h>
#include <sched.h>
#include <pthread.h>
#include <stdint.h>

#include "memoryCommon.h"

typedef struct {
    pthread_t thread;
    pthread_attr_t attr;
    struct sched_param threadParams;
    cpu_set_t cpuset;
    memory_threadArgs_t args;
} memory_runner_thread_vars_t;

void runMemoryBench(int *cpus, int numFIFOs, char* reportFilename, void* (*memory_thread_fun)(void*));

#endif