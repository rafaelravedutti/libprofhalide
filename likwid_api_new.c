#include <likwid.h>
#include <string.h>
//---
#include "perfctr_halide.h"

char current_marker[128];
int marker_active = 0;
int in_parallel_region = 0;

int perfctr_halide_initialize() {
    LIKWID_MARKER_INIT;
    return 0;
}

int perfctr_halide_start_thread() {
    if(in_parallel_region) {
        LIKWID_MARKER_THREADINIT;

        if(marker_active) {
            LIKWID_MARKER_START(current_marker);
        }
    }

    return 0;
}

int perfctr_halide_stop_thread() {
    if(in_parallel_region && marker_active) {
        LIKWID_MARKER_STOP(current_marker);
    }

    return 0;
}

int perfctr_halide_number_of_events() { return 0; }

int perfctr_halide_enter_parallel_region() {
    in_parallel_region = 1;

    if(marker_active) {
        LIKWID_MARKER_STOP(current_marker);
    }

    return 0;
}

int perfctr_halide_leave_parallel_region() {
    in_parallel_region = 0;

    if(marker_active) {
        LIKWID_MARKER_START(current_marker);
    }

    return 0;
}

int perfctr_halide_marker_start(const char *marker) {
    LIKWID_MARKER_START(marker);
    strncpy(current_marker, marker, sizeof current_marker);
    marker_active = 1;
    return 0;
}

int perfctr_halide_marker_stop(const char *marker, long long int *values, int accum) {
    LIKWID_MARKER_STOP(marker);
    marker_active = 0;
    return 0;
}

void perfctr_halide_shutdown() {
    LIKWID_MARKER_CLOSE;
}
