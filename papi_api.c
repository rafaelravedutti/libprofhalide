#include <papi.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ptrace.h>
#include <sys/time.h>
//---
#include "papi_api.h"
#include "papi_halide.h"
#include "transform.h"

static struct papi_api_config *global_config = NULL;
static struct papi_api_state *global_state = NULL;

pthread_mutex_t lock;

struct threadInfo {
  int event_set;
  int thread_id;
  int set;
  int level;
};

static struct threadInfo threadsInfo[MAX_PAPI_THREADS];

int papi_halide_get_thread_index() {
  int thid = pthread_self();

  for(int i = 0; i < MAX_PAPI_THREADS; ++i) {
    if(threadsInfo[i].set && threadsInfo[i].thread_id == thid) {
      return i;
    }
  }

  return -1;
}

int find_or_insert_thread() {
  int thid = pthread_self();
  int first_available_idx = -1;

  pthread_mutex_lock(&lock);

  for(int i = 0; i < MAX_PAPI_THREADS; ++i) {
    if(threadsInfo[i].set && threadsInfo[i].thread_id == thid) {
      pthread_mutex_unlock(&lock);
      return i;
    }

    if(!threadsInfo[i].set && first_available_idx == -1) {
      first_available_idx = i;
    }
  }

  if(first_available_idx == -1) {
    fprintf(stderr, "find_or_insert_thread(): No thread slot available!\n");
    return -1;
  }

  threadsInfo[first_available_idx].event_set = PAPI_NULL;
  threadsInfo[first_available_idx].thread_id = thid;
  threadsInfo[first_available_idx].level = global_state->parallel_level;
  threadsInfo[first_available_idx].set = 1;

  pthread_mutex_unlock(&lock);

  return first_available_idx;
}

int remove_threads() {
  for(int i = 0; i < MAX_PAPI_THREADS; ++i) {
    if(threadsInfo[i].set && threadsInfo[i].level > global_state->parallel_level) {
      threadsInfo[i].set = 0;
      threadsInfo[i].event_set = PAPI_NULL;
    }
  }

  return -1;
}

double timestamp() {
  struct timeval tp;

  gettimeofday(&tp, NULL);
  return ((double)(tp.tv_sec + tp.tv_usec / 1000000.0));
}

struct papi_api_event *new_event(int event_id, long long int (*transform)(long long int), int value_type) {
  struct papi_api_event *event;

  event = (struct papi_api_event *) malloc(sizeof(struct papi_api_event));

  if(event == NULL) {
    fprintf(stderr, "new_event(): Failed to allocate papi_api_event!\n");
    return NULL;
  }

  event->event_id = event_id;
  event->event_transform = transform;
  event->event_value_type = value_type;
  event->next = NULL;

  return event;
}

struct papi_api_event *get_event(const char *event_name) {
  int event_id, ret;

  if((ret = PAPI_event_name_to_code(event_name, &event_id)) != PAPI_OK) {
    fprintf(stderr, "PAPI_event_name_to_code(%s): %d\n", event_name, ret);
    return NULL;
  }

  return new_event(event_id, transform_skip, VALUE_TYPE_INTEGER);
}

struct papi_api_config *get_papi_api_config() {
  FILE *fp;
  struct papi_api_config *config;
  char config_line[MAX_CONFIG_LINE];
  char *line = NULL;
  size_t len, i, j, eq;
  ssize_t read;

  if((fp = fopen(PAPI_HALIDE_CONFIG_FILE, "r")) == NULL) {
    fprintf(stderr, "Configuration file for papi_halide not found: \"%s\"\n", PAPI_HALIDE_CONFIG_FILE);
    return NULL;
  }

  config = (struct papi_api_config *) malloc(sizeof(struct papi_api_config));

  if(config != NULL) {
    while((read = getline(&line, &len, fp)) > 0) {
      for(i = 0, j = 0, eq = -1; i < len; ++i) {
        if(line[i] != ' ' && line[i] != '\n') {
          if(line[i] == '=' && eq == -1) {
            eq = j;
          }

          config_line[j++] = line[i];
        }
      }

      config_line[j] = '\0';

      if(strncmp(config_line, "events", strlen("events")) == 0) {
        strncpy(config->events, config_line + eq + 1, j - eq);
        config->events[j - eq] = '\0';
      }
    }

    free(line);
  }

  return config;
}

int build_events(struct papi_api_event **events, int *event_set, struct papi_api_config *config) {
  struct papi_api_event *event, *last_event;;
  char *event_name;
  int counter;

  last_event = NULL;
  counter = 0;
  event_name = strtok(config->events, ",");

  while(event_name != NULL) {
    event = get_event(event_name);

    if(event != NULL) {
      if(*events == NULL) {
        *events = event;
      }

      event_set[counter] = event->event_id;
      counter += 1;

      if(last_event != NULL) {
        last_event->next = event;
      }

      last_event = event;
    }

    event_name = strtok(NULL, ",");
  }

  return counter;
}

void print_event_values(const char *func_name, long long int *values, struct papi_api_event *events, double time) {
  struct papi_api_event *event;
  long long int tvalue, tvalue2;
  unsigned int i;

  fprintf(stdout, "%s: ", func_name);

  for(event = events, i = 0; event != NULL; event = event->next, ++i) {
    tvalue = event->event_transform(values[i]);

    if(i != 0) {
      fprintf(stdout, ", ");
    }

    switch(event->event_value_type) {
      case VALUE_TYPE_INTEGER:
        fprintf(stdout, "%lld", tvalue);
        break;
      case VALUE_TYPE_TWO_INTEGERS:
        tvalue2 = event->event_transform(values[i + 1]);
        fprintf(stdout, "%lld, %lld", tvalue, tvalue2);
        break;
      case VALUE_TYPE_FLOAT:
        fprintf(stdout, "%.4f", *((float *) &tvalue));
        break;
      default:
        fprintf(stdout, "%lld", tvalue);
    }
  }

  fprintf(stdout, " (time = %.8g)\n", time);
}

int papi_halide_initialize() {
  int ret;

  if(pthread_mutex_init(&lock, NULL) != 0) {
    fprintf(stderr, "pthread_mutex_init(): Failed to initialize mutex!\n");
    return -1;
  }

  global_state = (struct papi_api_state *) malloc(sizeof(struct papi_api_state));

  if(global_state == NULL) {
    fprintf(stderr, "papi_marker_start(): Failed to allocate global state!\n");
    return -1;
  }

  if((ret = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
    fprintf(stderr, "PAPI_library_init(): %d", ret);
    return -1;
  }

  global_state->parallel_level = 0;
  global_state->event_set = -1;
  global_state->events = NULL;

  global_config = get_papi_api_config();
  global_state->num_events = build_events(&(global_state->events), global_state->event_array, global_config);

  if((ret = PAPI_thread_init((unsigned long (*)(void))(pthread_self))) != PAPI_OK) {
    fprintf(stderr, "PAPI_thread_init(): %d\n", ret);
    return -1;
  }

  return 0;
}

int papi_halide_number_of_events() {
  return global_state->num_events;
}

int papi_halide_start_thread() {
  int thread_idx = find_or_insert_thread();
  int ret;

  if((ret = PAPI_register_thread()) != PAPI_OK) {
    fprintf(stderr, "PAPI_register_thread(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_create_eventset(&threadsInfo[thread_idx].event_set)) != PAPI_OK) {
    fprintf(stderr, "PAPI_create_eventset(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_add_events(threadsInfo[thread_idx].event_set, global_state->event_array, global_state->num_events)) != PAPI_OK) {
    fprintf(stderr, "PAPI_add_events(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_start(threadsInfo[thread_idx].event_set)) != PAPI_OK) {
    fprintf(stderr, "PAPI_start(): %d\n", ret);
    return -1;
  }

  return 0;
}

int papi_halide_stop_thread() {
  long long int dummyvalues[MAX_PAPI_EVENTS];
  int thread_idx = papi_halide_get_thread_index();
  int ret;

  if(thread_idx == -1) {
    fprintf(stderr, "papi_halide_stop_thread(): Thread not found!\n");
    return -1;
  }

  if((ret = PAPI_stop(threadsInfo[thread_idx].event_set, dummyvalues)) != PAPI_OK) {
    fprintf(stderr, "PAPI_stop(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_remove_events(threadsInfo[thread_idx].event_set, global_state->event_array, global_state->num_events)) != PAPI_OK) {
    fprintf(stderr, "PAPI_remove_events(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_destroy_eventset(&threadsInfo[thread_idx].event_set)) != PAPI_OK) {
    fprintf(stderr, "PAPI_destroy_eventset(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_unregister_thread()) != PAPI_OK) {
    fprintf(stderr, "PAPI_unregister_thread(): %d\n", ret);
    return -1;
  }

  return 0;
}

int papi_halide_enter_parallel_region() {
  global_state->parallel_level++;
  return 0;
}

int papi_halide_leave_parallel_region() {
  global_state->parallel_level--;
  remove_threads();
  return 0;
}

int papi_halide_marker_start() {
  int thread_idx = papi_halide_get_thread_index();

  PAPI_reset(threadsInfo[thread_idx].event_set);
  return 0;
}

int papi_halide_marker_stop(long long int *values, int accum) {
  int thread_idx = papi_halide_get_thread_index();

  PAPI_accum(threadsInfo[thread_idx].event_set, values);
  return 0;
}

void papi_halide_shutdown() {
  struct papi_api_event *event, *previous;

  event = NULL;
  previous = global_state->events;

  if(previous != NULL) {
    event = previous->next;
  }

  while(event != NULL) {
    free(previous);
    previous = event;
    event = event->next;
  }

  if(previous != NULL) {
    free(previous);
  }

  free(global_config);
  free(global_state);

  pthread_mutex_destroy(&lock);

  PAPI_shutdown();
}
