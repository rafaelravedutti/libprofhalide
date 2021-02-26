#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ptrace.h>
#include <sys/time.h>
//---
#include "perfctr_halide.h"

#define NUM_MARKER_TYPES        4
#define MAX_MARKERS_PER_TYPE    4
#define MAX_THREADS             32
#define MAX_MARKER_NAME_LEN     128

pthread_mutex_t lock;

struct threadInfo {
    int thread_id;
    int set;
};

static clock_t start_times[MAX_THREADS];
static double timers_accum[NUM_MARKER_TYPES][MAX_MARKERS_PER_TYPE][MAX_THREADS];
static bool timers_accum_flag[NUM_MARKER_TYPES][MAX_MARKERS_PER_TYPE][MAX_THREADS];
static char marker_names[NUM_MARKER_TYPES][MAX_MARKERS_PER_TYPE][MAX_MARKER_NAME_LEN];
static struct threadInfo threadsInfo[MAX_THREADS];

int find_or_insert_thread() {
    int thid = pthread_self();
    int first_available_idx = -1;

    for(int i = 0; i < MAX_THREADS; ++i) {
        if(threadsInfo[i].set && threadsInfo[i].thread_id == thid) {
            return i;
        }
    }

    pthread_mutex_lock(&lock);
    for(int i = 0; i < MAX_THREADS; ++i) {
        if(!threadsInfo[i].set && first_available_idx == -1) {
            first_available_idx = i;
        }
    }

    if(first_available_idx == -1) {
        fprintf(stderr, "find_or_insert_thread(): No thread slot available!\n");
        return -1;
    }

    threadsInfo[first_available_idx].thread_id = thid;
    threadsInfo[first_available_idx].set = 1;
    pthread_mutex_unlock(&lock);
    return first_available_idx;
}

int perfctr_halide_initialize() {
    if(pthread_mutex_init(&lock, NULL) != 0) {
        fprintf(stderr, "pthread_mutex_init(): Failed to initialize mutex!\n");
        return -1;
    }

    for(int t = 0; t < NUM_MARKER_TYPES; t++) {
        for(int m = 0; m < MAX_MARKERS_PER_TYPE; m++) {
            for(int th = 0; th < MAX_THREADS; th++) {
                timers_accum[t][m][th] = 0;
                timers_accum_flag[t][m][th] = false;
            }
        }
    }

    return 0;
}

int perfctr_halide_marker_register(const char *marker, int id, int type) {
    strncpy(marker_names[type][id], marker, strlen(marker));
    return 0;
}

int perfctr_halide_marker_start(const char *marker, int id, int type) {
    int thread_idx = find_or_insert_thread();
    start_times[thread_idx] = clock();
    return 0;
}

int perfctr_halide_marker_stop(const char *marker, int id, int type) {
    clock_t t = clock();
    int thread_idx = find_or_insert_thread();
    timers_accum[type][id][thread_idx] += (double)(t - start_times[thread_idx]) / CLOCKS_PER_SEC;
    timers_accum_flag[type][id][thread_idx] = true;
    return 0;
}

void perfctr_halide_shutdown() {
    double total_time = 0.0;

    for(int t = 0; t < NUM_MARKER_TYPES; t++) {
        for(int m = 0; m < MAX_MARKERS_PER_TYPE; m++) {
            double tmax = 0.0;
            bool any = false;

            for(int th = 0; th < MAX_THREADS; th++) {
                if(timers_accum_flag[t][m][th]) {
                    double time = timers_accum[t][m][th];

                    if(tmax < time) {
                        tmax = time;
                    }

                    any = true;
                    fprintf(stdout, "%s,t%d,%f\n", marker_names[t][m], th, time);
                }
            }

            if(any) {
                fprintf(stdout, "%s,max,%f\n", marker_names[t][m], tmax);
                total_time += tmax;
            }
        }
    }

    fprintf(stdout, "total,%f\n", total_time);
    pthread_mutex_destroy(&lock);
}
