#include <likwid.h>
//---
#include "perfctr_halide.h"

int perfctr_halide_initialize() {
    LIKWID_MARKER_INIT;
    return 0;
}

int perfctr_halide_start_thread() {
    LIKWID_MARKER_THREADINIT;
    return 0;
}

int perfctr_halide_stop_thread() { return 0; }
int perfctr_halide_number_of_events() { return 0; }
int perfctr_halide_enter_parallel_region() { return 0; }
int perfctr_halide_leave_parallel_region() { return 0; }

int perfctr_halide_marker_start(const char *marker) {
    LIKWID_MARKER_START(marker);
    return 0;
}

int perfctr_halide_marker_stop(const char *marker, long long int *values, int accum) {
    LIKWID_MARKER_STOP(marker);
    return 0;
}

void perfctr_halide_shutdown() {
    LIKWID_MARKER_CLOSE;
}
