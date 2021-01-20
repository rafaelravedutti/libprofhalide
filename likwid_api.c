#include <likwid.h>
#include <pthread.h>
#include <stdio.h>
//---
#include "perfctr_halide.h"

#define MAX_THREADS 128

int threadMap[MAX_THREADS];
int nProcessors = 0;
pthread_mutex_t lock;

int perfctr_halide_initialize() {
    LIKWID_MARKER_INIT;
    return 0;
}

int perfctr_halide_start_thread() {
    LIKWID_MARKER_THREADINIT;

    /*
    fprintf(stdout, "perfctr_halide_start_thread(): %ld\n", pthread_self());
    int thread_id = pthread_self();
    int i;

    pthread_mutex_lock(&lock);

    for(i = 0; i < nProcessors; ++i) {
        if(threadMap[i] == thread_id) {
            break;
        }
    }

    if(i == nProcessors) {
        likwid_pinThread(nProcessors);
        threadMap[nProcessors++] = thread_id;
    }

    pthread_mutex_unlock(&lock);
    */
    return 0;
}

int perfctr_halide_stop_thread() { return 0; }
int perfctr_halide_number_of_events() { return 0; }
int perfctr_halide_enter_parallel_region() { return 0; }
int perfctr_halide_leave_parallel_region() { return 0; }

int perfctr_halide_marker_start(const char *marker) {
    pthread_mutex_lock(&lock);
    int myCPU = likwid_getProcessorId();
    fprintf(stdout, "perfctr_halide_marker_start(\"%s\"): pthread_self, %ld, myCPU: %d\n", marker, pthread_self(), myCPU);
    LIKWID_MARKER_START(marker);
    pthread_mutex_unlock(&lock);
    return 0;
}

int perfctr_halide_marker_stop(const char *marker, long long int *values, int accum) {
    pthread_mutex_lock(&lock);
    int myCPU = likwid_getProcessorId();
    fprintf(stdout, "perfctr_halide_marker_stop(\"%s\"): pthread_self, %ld, myCPU: %d\n", marker, pthread_self(), myCPU);
    LIKWID_MARKER_STOP(marker);
    pthread_mutex_unlock(&lock);
    return 0;
}

void perfctr_halide_shutdown() {
    LIKWID_MARKER_CLOSE;
}
