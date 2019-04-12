#include "HalideRuntime.h"
#include "printer.h"
#include "scoped_mutex_lock.h"
#include "papi_profiler.h"

extern "C" {

static halide_papi_pipeline_stats *current_pipeline_stats = NULL;

// Returns the address of the global halide_profiler state
WEAK halide_papi_state *halide_papi_get_state() {
    static halide_papi_state s = {0, NULL};
    return &s;
}

WEAK int halide_papi_pipeline_start(void *user_context, const char *pipeline_name, int num_funcs, const uint64_t *func_names) {
    halide_papi_state *s = halide_papi_get_state();
    halide_papi_pipeline_stats *p;

    for(p = s->pipelines; p; p = (halide_papi_pipeline_stats *)(p->next)) {
      if(p->name == pipeline_name && p->num_funcs == num_funcs) {
        break;
      }
    }

    if(!p) {
      p = (halide_papi_pipeline_stats *) malloc(sizeof(halide_papi_pipeline_stats));

      if(p) {
        p->name = pipeline_name;
        p->next = s->pipelines;
        p->num_funcs = num_funcs;
        p->funcs = (halide_papi_func_stats *) malloc(num_funcs * sizeof(halide_papi_func_stats));

        if(p->funcs != NULL) {
          for(int i = 0; i < num_funcs; ++i) {
            p->funcs[i].name = (const char *)(func_names[i]);
          }
        } else {
          free(p);
          p = NULL;
        }
      }

      if(!p) {
        return -1;
      }

      s->pipelines = p;
    }

    current_pipeline_stats = p;
    papi_halide_initialize();
    return 0;
}

WEAK void halide_papi_stack_peak_update(void *user_context, void *pipeline_state, uint64_t *f_values) {

}

WEAK void halide_papi_memory_allocate(void *user_context, void *pipeline_state, int func_id, uint64_t incr) {

}

WEAK void halide_papi_memory_free(void *user_context, void *pipeline_state, int func_id, uint64_t decr) {

}

WEAK void halide_papi_report_unlocked(void *user_context, halide_profiler_state *s) {

}

WEAK void halide_papi_report(void *user_context) {

}

WEAK void halide_papi_reset_unlocked(halide_profiler_state *s) {

}

WEAK void halide_papi_reset() {

}

WEAK void halide_papi_shutdown() {
  halide_papi_state *s = halide_papi_get_state();
  halide_papi_pipeline_stats *p, *tmp;

  p = s->pipelines;

  while(p != NULL) {
    tmp = (halide_papi_pipeline_stats *)(p->next);

    free(p->funcs);
    free(p);

    p = tmp;
  }

  papi_halide_shutdown();
}

WEAK void halide_papi_pipeline_end(void *user_context, void *state) {
  current_pipeline_stats = NULL;
}

WEAK __attribute__((always_inline)) int halide_papi_set_current_func(halide_papi_state *state, int tok, int t) {
    papi_halide_marker_start(t, current_pipeline_stats->funcs[t].name);
    return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_leave_current_func(halide_papi_state *state, int tok, int t) {
    papi_halide_marker_stop(t, current_pipeline_stats->funcs[t].name);
    return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_incr_active_threads(halide_papi_state *state) {
    return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_decr_active_threads(halide_papi_state *state) {
    return 0;
}


} // extern "C"
