#include "HalideRuntime.h"
#include "printer.h"
#include "scoped_mutex_lock.h"
#include "papi_profiler.h"

extern "C" {

static halide_papi_pipeline_stats *current_pipeline_stats = NULL;

namespace Halide { namespace Runtime { namespace Internal {

WEAK void bill_func(halide_papi_state *s, int func_id, uint64_t time, int active_threads) {
  halide_papi_pipeline_stats *p_prev = NULL;
  for (halide_papi_pipeline_stats *p = s->pipelines; p;
       p = (halide_papi_pipeline_stats *)(p->next)) {
      if (func_id >= p->first_func_id && func_id < p->first_func_id + p->num_funcs) {
          if (p_prev) {
              // Bubble the pipeline to the top to speed up future queries.
              p_prev->next = (halide_papi_pipeline_stats *)(p->next);
              p->next = s->pipelines;
              s->pipelines = p;
          }
          halide_papi_func_stats *f = p->funcs + func_id - p->first_func_id;
          f->time += time;
          f->active_threads_numerator += active_threads;
          f->active_threads_denominator += 1;
          p->time += time;
          p->samples++;
          p->active_threads_numerator += active_threads;
          p->active_threads_denominator += 1;
          return;
      }
      p_prev = p;
  }
  // Someone must have called reset_state while a kernel was running. Do nothing.
}

WEAK void sampling_profiler_thread(void *) {
  halide_papi_state *s = halide_papi_get_state();

  // grab the lock
  halide_mutex_lock(&s->lock);

  while (s->current_func != halide_papi_please_stop) {
      uint64_t t1 = halide_current_time_ns(NULL);
      uint64_t t = t1;
      while (1) {
          int func, active_threads;
          if (s->get_remote_profiler_state) {
              // Execution has disappeared into remote code running
              // on an accelerator (e.g. Hexagon DSP)
              s->get_remote_profiler_state(&func, &active_threads);
          } else {
              func = s->current_func;
              active_threads = s->active_threads;
          }
          uint64_t t_now = halide_current_time_ns(NULL);
          if (func == halide_papi_please_stop) {
              break;
          } else if (func >= 0) {
              // Assume all time since I was last awake is due to
              // the currently running func.
              bill_func(s, func, t_now - t, active_threads);
          }
          t = t_now;

          // Release the lock, sleep, reacquire.
          int sleep_ms = s->sleep_time;
          halide_mutex_unlock(&s->lock);
          halide_sleep_ms(NULL, sleep_ms);
          halide_mutex_lock(&s->lock);
      }
  }

  halide_mutex_unlock(&s->lock);
}

}}}

// Returns the address of the global halide_papi state
WEAK halide_papi_state *halide_papi_get_state() {
  static halide_papi_state s = {{{0}}, 1, 0, 0, 0, 0, NULL, NULL};
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
      p->first_func_id = s->first_free_id;
      p->num_funcs = num_funcs;
      p->runs = 0;
      p->time = 0;
      p->samples = 0;
      p->active_threads_numerator = 0;
      p->active_threads_denominator = 0;
      p->funcs = (halide_papi_func_stats *) malloc(num_funcs * sizeof(halide_papi_func_stats));

      if(p->funcs != NULL) {
        for(int i = 0; i < num_funcs; ++i) {
          p->funcs[i].time = 0;
          p->funcs[i].name = (const char *)(func_names[i]);
          p->funcs[i].active_threads_numerator = 0;
          p->funcs[i].active_threads_denominator = 0;
        }
      } else {
        free(p);
        p = NULL;
      }
    }

    if(!p) {
      return -1;
    }

    s->first_free_id += num_funcs;
    s->pipelines = p;
  }

  ScopedMutexLock lock(&s->lock);

  if (!s->sampling_thread) {
      halide_start_clock(user_context);
      s->sampling_thread = halide_spawn_thread(sampling_profiler_thread, NULL);
  }

  current_pipeline_stats = p;
  papi_halide_initialize();
  return p->first_func_id;
}

/*
WEAK void halide_papi_stack_peak_update(void *user_context, void *pipeline_state, uint64_t *f_values) {

}

WEAK void halide_papi_memory_allocate(void *user_context, void *pipeline_state, int func_id, uint64_t incr) {

}

WEAK void halide_papi_memory_free(void *user_context, void *pipeline_state, int func_id, uint64_t decr) {

}
*/

WEAK void halide_papi_report_unlocked(void *user_context, halide_papi_state *s) {

}

WEAK void halide_papi_report(void *user_context) {
  halide_papi_state *s = halide_papi_get_state();
  ScopedMutexLock lock(&s->lock);
  halide_papi_report_unlocked(user_context, s);
}

WEAK void halide_papi_reset_unlocked(halide_papi_state *s) {
  while (s->pipelines) {
    halide_papi_pipeline_stats *p = s->pipelines;
    s->pipelines = (halide_papi_pipeline_stats *)(p->next);
    free(p->funcs);
    free(p);
  }

  s->first_free_id = 0;
}

WEAK void halide_papi_reset() {
  // WARNING: Do not call this method while any other halide
  // pipeline is running; halide_papi_memory_allocate/free and
  // halide_papi_stack_peak_update update the profiler pipeline's
  // state without grabbing the global profiler state's lock.
  halide_papi_state *s = halide_papi_get_state();
  ScopedMutexLock lock(&s->lock);
  halide_papi_reset_unlocked(s);
}

WEAK void halide_papi_shutdown() {
  halide_papi_state *s = halide_papi_get_state();

  if (!s->sampling_thread) {
      return;
  }

  s->current_func = halide_papi_please_stop;
  halide_join_thread(s->sampling_thread);
  s->sampling_thread = NULL;
  s->current_func = halide_papi_outside_of_halide;

  // Print results. No need to lock anything because we just shut
  // down the thread.
  halide_papi_report_unlocked(NULL, s);

  halide_papi_reset_unlocked(s);
}

WEAK void halide_papi_pipeline_end(void *user_context, void *state) {
  papi_halide_shutdown();
  current_pipeline_stats = NULL;
}

WEAK __attribute__((always_inline)) int halide_papi_set_current_func(halide_papi_state *state, int tok, int t) {
  // Use empty volatile asm blocks to prevent code motion. Otherwise
  // llvm reorders or elides the stores.
  volatile int *ptr = &(state->current_func);
  asm volatile ("":::);
  *ptr = tok + t;
  asm volatile ("":::);

  papi_halide_marker_start(t, current_pipeline_stats->funcs[t].name);
  return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_leave_current_func(halide_papi_state *state, int tok, int t) {
  papi_halide_marker_stop(t, current_pipeline_stats->funcs[t].name);
  return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_incr_active_threads(halide_papi_state *state) {
  volatile int *ptr = &(state->active_threads);
  asm volatile ("":::);
  int ret = __sync_fetch_and_add(ptr, 1);
  asm volatile ("":::);
  return ret;
}

WEAK __attribute__((always_inline)) int halide_papi_decr_active_threads(halide_papi_state *state) {
  volatile int *ptr = &(state->active_threads);
  asm volatile ("":::);
  int ret = __sync_fetch_and_sub(ptr, 1);
  asm volatile ("":::);
  return ret;
}


} // extern "C"
