#include <likwid.h>
#include <pthread.h>
#include <stdio.h>
//---
#include "perfctr_halide.h"

int perfctr_halide_initialize() {
    LIKWID_MARKER_INIT;
    return 0;
}

int perfctr_halide_marker_register(const char *marker, int id, int type) {
    LIKWID_MARKER_REGISTER(marker);
    return 0;
}

int perfctr_halide_marker_start(const char *marker, int id, int type) {
    LIKWID_MARKER_START(marker);
    return 0;
}

int perfctr_halide_marker_stop(const char *marker, int id, int type) {
    LIKWID_MARKER_STOP(marker);
    return 0;
}

void perfctr_halide_shutdown() {
    LIKWID_MARKER_CLOSE;
}
