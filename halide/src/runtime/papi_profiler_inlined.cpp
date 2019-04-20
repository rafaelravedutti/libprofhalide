#include "HalideRuntime.h"
#include "papi_profiler.h"

extern "C" {

WEAK __attribute__((always_inline)) int halide_papi_incr_active_threads(halide_papi_state *state) {
    volatile int *ptr = &(state->active_threads);
    asm volatile ("":::);
    int ret = __sync_fetch_and_add(ptr, 1);
    asm volatile ("":::);

    papi_halide_start_thread();
    return ret;
}

WEAK __attribute__((always_inline)) int halide_papi_decr_active_threads(halide_papi_state *state) {
    volatile int *ptr = &(state->active_threads);
    asm volatile ("":::);
    int ret = __sync_fetch_and_sub(ptr, 1);
    asm volatile ("":::);

    papi_halide_stop_thread();
    return ret;
}

}
