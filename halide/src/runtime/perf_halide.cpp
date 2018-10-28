#include "HalideRuntime.h"
#include "printer.h"
#include "scoped_mutex_lock.h"
#include "perf_halide.h"

extern "C" {

// Returns the address of the global halide_profiler state
WEAK perf_halide_state *perf_halide_get_state() {
    static perf_halide_state s = {0};
    return &s;
}

WEAK int perf_halide_pipeline_start(void *user_context, const char *pipeline_name, int num_funcs, const uint64_t *func_names) {
    papi_marker_start();
    return 0;
}

WEAK void perf_halide_stack_peak_update(void *user_context, void *pipeline_state, uint64_t *f_values) {

}

WEAK void perf_halide_memory_allocate(void *user_context, void *pipeline_state, int func_id, uint64_t incr) {

}

WEAK void perf_halide_memory_free(void *user_context, void *pipeline_state, int func_id, uint64_t decr) {

}

WEAK void perf_halide_report_unlocked(void *user_context, halide_profiler_state *s) {

}

WEAK void perf_halide_report(void *user_context) {

}


WEAK void perf_halide_reset_unlocked(halide_profiler_state *s) {

}

WEAK void perf_halide_reset() {

}

WEAK void perf_halide_shutdown() {

}

WEAK void perf_halide_pipeline_end(void *user_context, void *state) {
    papi_marker_stop();
}

WEAK __attribute__((always_inline)) int perf_halide_set_current_func(perf_halide_state *state, int tok, int t) {
    return 0;
}

WEAK __attribute__((always_inline)) int perf_halide_incr_active_threads(perf_halide_state *state) {
    return 0;
}

WEAK __attribute__((always_inline)) int perf_halide_decr_active_threads(perf_halide_state *state) {
    return 0;
}


} // extern "C"
