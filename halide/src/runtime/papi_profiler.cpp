#include "HalideRuntime.h"
#include "printer.h"
#include "scoped_mutex_lock.h"
#include "papi_profiler.h"

#define PROFILE_GRANULARITY   100

extern "C" {

namespace Halide { namespace Runtime { namespace Internal {

static halide_papi_pipeline_stats *current_pipeline_stats = NULL;

WEAK void bill_func(halide_papi_state *s, int func_id, uint64_t time, int active_threads) {
  halide_papi_pipeline_stats *p_prev = NULL;
  for(halide_papi_pipeline_stats *p = s->pipelines; p; p = (halide_papi_pipeline_stats *)(p->next)) {
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

  while(s->current_func != halide_papi_please_stop) {
    uint64_t t1 = halide_current_time_ns(NULL);
    uint64_t t = t1;
    while(1) {
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

WEAK int halide_papi_pipeline_start(
  void *user_context,
  const char *pipeline_name,
  int num_funcs,
  const uint64_t *func_names,
  const int64_t *func_parents_prod,
  const int64_t *func_parents_cons,
  const uint64_t *func_show_threads_prod,
  const uint64_t *func_show_threads_cons) {

  halide_papi_state *s = halide_papi_get_state();
  halide_papi_pipeline_stats *p;

  for(p = s->pipelines; p; p = (halide_papi_pipeline_stats *)(p->next)) {
    if(p->name == pipeline_name && p->num_funcs == num_funcs) {
      break;
    }
  }

  if(!p) {
    p = (halide_papi_pipeline_stats *) malloc(sizeof(halide_papi_pipeline_stats));
  }

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
        p->funcs[i].parent_prod = (int)(func_parents_prod[i]);
        p->funcs[i].parent_cons = (int)(func_parents_cons[i]);
        p->funcs[i].show_threads_prod = (bool)(func_show_threads_prod[i]);
        p->funcs[i].show_threads_cons = (bool)(func_show_threads_cons[i]);

        for(int l = 0; l < 2; ++l) {
          p->funcs[i].clock_start[l] = 0;
          p->funcs[i].clock_accum[l] = 0;

          for(int t = 0; t < MAX_PAPI_THREADS; ++t) {
            p->funcs[i].counter_used[l][t] = 0;
            p->funcs[i].overhead_counter_used[t] = 0;

            for(int e = 0; e < MAX_PAPI_DESCRIPTORS; ++e) {
              p->funcs[i].overhead_counters[t][e] = 0;
              p->funcs[i].event_counters[l][t][e] = 0;
            }
          }
        }
      }
    } else {
      free(p);
      p = NULL;
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

  p->runs++;

  current_pipeline_stats = p;
  papi_halide_initialize();
  return p->first_func_id;
}

WEAK void halide_papi_report_func_prod_cons(void *user_context, halide_papi_func_stats *fs, bool is_prod, uint64_t level) {
  char line_buf[1024];
  Printer<StringStreamPrinter, sizeof(line_buf)> sstr(user_context, line_buf);
  long long total_events[MAX_PAPI_EVENTS];
  int stage = (is_prod) ? 0 : 1;
  bool show_threads = (is_prod) ? fs->show_threads_prod : fs->show_threads_cons;
  size_t active_threads = 0;

  for(int e = 0; e < papi_halide_number_of_events(); ++e) {
    total_events[e] = 0;
  }

  for(int th = 0; th < MAX_PAPI_THREADS; ++th) {
    if(fs->counter_used[stage][th] == 1) {
      active_threads++;

      for(int e = 0; e < papi_halide_number_of_events(); ++e) {
        total_events[e] += fs->event_counters[stage][th][e];
      }

      if(show_threads) {
        sstr.clear();
        sstr << fs->name;

        if(is_prod) {
          sstr << "_prod";
        } else {
          sstr << "_cons";
        }

        sstr << "_t" << th << ",";

        for(int e = 0; e < papi_halide_number_of_events(); ++e) {
          sstr << (fs->event_counters[stage][th][e] * PROFILE_GRANULARITY) << ",";
        }

        sstr.erase(1);
        sstr << "\n";

        halide_print(user_context, sstr.str());
      }
    }
  }

  if(!show_threads || active_threads > 1) {
    sstr.clear();
    sstr << fs->name;

    if(is_prod) {
      sstr << "_prod_";
    } else {
      sstr << "_cons_";
    }

    if(active_threads > 1) {
      sstr << "total,";
    } else {
      sstr << "t0,";
    }

    for(int e = 0; e < papi_halide_number_of_events(); ++e) {
      sstr << (total_events[e] * PROFILE_GRANULARITY) << ",";
    }

    sstr.erase(1);
    sstr << "\n";

    halide_print(user_context, sstr.str());
  }
}

WEAK void halide_papi_report_func_overhead(void *user_context, halide_papi_func_stats *fs, uint64_t level) {
  char line_buf[1024];
  Printer<StringStreamPrinter, sizeof(line_buf)> sstr(user_context, line_buf);
  long long total_events[MAX_PAPI_EVENTS];
  size_t active_threads = 0;

  for(int e = 0; e < papi_halide_number_of_events(); ++e) {
    total_events[e] = 0;
  }

  for(int th = 0; th < MAX_PAPI_THREADS; ++th) {
    if(fs->overhead_counter_used[th] == 1) {
      active_threads++;

      for(int e = 0; e < papi_halide_number_of_events(); ++e) {
        total_events[e] += fs->overhead_counters[th][e];
      }

      if(fs->show_threads_prod) {
        sstr.clear();
        sstr << fs->name << "_overhead" << "_t" << th << ",";

        for(int e = 0; e < papi_halide_number_of_events(); ++e) {
          sstr << (fs->overhead_counters[th][e] * PROFILE_GRANULARITY) << ",";
        }

        sstr.erase(1);
        sstr << "\n";

        halide_print(user_context, sstr.str());
      }
    }
  }

  if(!fs->show_threads_prod || active_threads > 1) {
    sstr.clear();
    sstr << fs->name << "_overhead_";

    if(active_threads > 1) {
      sstr << "total,";
    } else {
      sstr << "t0,";
    }

    for(int e = 0; e < papi_halide_number_of_events(); ++e) {
      sstr << (total_events[e] * PROFILE_GRANULARITY) << ",";
    }

    sstr.erase(1);
    sstr << "\n";

    halide_print(user_context, sstr.str());
  }
}

WEAK void halide_papi_report_counters(void *user_context, halide_papi_pipeline_stats *pipeline_stats, int parent, uint64_t level) {
  for(int i = 0; i < pipeline_stats->num_funcs; i++) {
    halide_papi_func_stats *fs = pipeline_stats->funcs + i;

    //if(fs->parent_prod != parent && fs->parent_cons != parent) continue;

    // The first func is always a catch-all overhead
    // slot. Only report overhead time if it's non-zero
    if(i == 0 && fs->time == 0) continue;

    //if(fs->parent_prod == parent) {
      halide_papi_report_func_overhead(user_context, fs, level);
      halide_papi_report_func_prod_cons(user_context, fs, true, level);
    //}

    //if(fs->parent_cons == parent) {
      halide_papi_report_func_prod_cons(user_context, fs, false, level);
    //}

    //halide_papi_report_scope(user_context, pipeline_stats, i, level + 1);
  }
}

WEAK void halide_papi_report_unlocked(void *user_context, halide_papi_state *s) {
  char line_buf[1024];
  Printer<StringStreamPrinter, sizeof(line_buf)> sstr(user_context, line_buf);

  for(halide_papi_pipeline_stats *p = s->pipelines; p; p = (halide_papi_pipeline_stats *)(p->next)) {
    float t = p->time / 1000000.0f;

    if (!p->runs) continue;

    sstr.clear();

    float threads = p->active_threads_numerator / (p->active_threads_denominator + 1e-10);

    sstr << "all," << t << "," << threads << "\n";

    for(int i = 0; i < p->num_funcs; i++) {
      halide_papi_func_stats *fs = p->funcs + i;
      float ft = fs->time / (p->runs * 1000000.0f);
      float fthreads = fs->active_threads_numerator / (fs->active_threads_denominator + 1e-10);

      sstr << fs->name << "," << ft << "," << fthreads << "\n";
    }

    sstr << "\n";

    halide_print(user_context, sstr.str());
    halide_papi_report_counters(user_context, p, 0, 0);
  }
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

__attribute__((destructor))  WEAK void halide_papi_shutdown() {
  halide_papi_state *s = halide_papi_get_state();

  if (!s->sampling_thread) {
      return;
  }

  s->current_func = halide_papi_please_stop;
  halide_join_thread(s->sampling_thread);
  s->sampling_thread = NULL;
  s->current_func = halide_papi_outside_of_halide;

  papi_halide_shutdown();

  // Print results. No need to lock anything because we just shut
  // down the thread.
  halide_papi_report_unlocked(NULL, s);

  halide_papi_reset_unlocked(s);
}

WEAK void halide_papi_pipeline_end(void *user_context, void *state) {
  ((halide_profiler_state *)state)->current_func = halide_profiler_outside_of_halide;
  halide_papi_shutdown();
  current_pipeline_stats = NULL;
}

WEAK __attribute__((always_inline)) int halide_papi_set_current_func(halide_papi_state *state, int tok, int t) {
  // Use empty volatile asm blocks to prevent code motion. Otherwise
  // llvm reorders or elides the stores.
  volatile int *ptr = &(state->current_func);
  asm volatile ("":::);
  *ptr = tok + t;
  asm volatile ("":::);
  return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_enter_current_func(halide_papi_state *state, int tok, int t, bool is_producer) {
  halide_papi_func_stats *fs = current_pipeline_stats->funcs + t;
  int level = is_producer ? 0 : 1;

  /*
  fs->clock_start[level] = halide_current_time_ns(NULL);
  */

  if(fs->iterations[level] % PROFILE_GRANULARITY == 0) {
    papi_halide_marker_start();
  }

  return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_leave_current_func(halide_papi_state *state, int tok, int t, bool is_producer) {
  halide_papi_func_stats *fs = current_pipeline_stats->funcs + t;
  int thread_idx = papi_halide_get_thread_index();
  int level = is_producer ? 0 : 1;

  if(fs->iterations[level] % PROFILE_GRANULARITY == 0) {
    papi_halide_marker_stop(fs->event_counters[level][thread_idx], fs->counter_used[level][thread_idx]);
    //fs->clock_accum[level] += halide_current_time_ns(NULL) - fs->clock_start[level];
    fs->counter_used[level][thread_idx] = 1;
  }

  fs->iterations[level]++;
  return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_enter_overhead_region(halide_papi_state *state, int tok, int t) {
  halide_papi_func_stats *fs = current_pipeline_stats->funcs + t;

  /*
  fs->overhead_clock_start = halide_current_time_ns(NULL);
  */

  if(fs->overhead_iterations % PROFILE_GRANULARITY == 0) {
    papi_halide_marker_start();
  }

  return 0;
}

WEAK __attribute__((always_inline)) int halide_papi_leave_overhead_region(halide_papi_state *state, int tok, int t) {
  halide_papi_func_stats *fs = current_pipeline_stats->funcs + t;
  int thread_idx = papi_halide_get_thread_index();

  if(fs->overhead_iterations % PROFILE_GRANULARITY == 0) {
    papi_halide_marker_stop(fs->overhead_counters[thread_idx], fs->overhead_counter_used[thread_idx]);
    //fs->overhead_clock_accum += halide_current_time_ns(NULL) - fs->overhead_clock_start;
    fs->overhead_counter_used[thread_idx] = 1;
  }

  fs->overhead_iterations++;
  return 0;
}

} // extern "C"
