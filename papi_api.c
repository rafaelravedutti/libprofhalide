#include <papi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wait.h>
#include <sys/ptrace.h>
//---
#include "papi_api.h"
#include "perf_halide.h"
#include "transform.h"

static int event_array[MAX_PAPI_EVENTS];

static struct papi_api_config *global_config = NULL;
static struct papi_api_state *global_state = NULL;

static pid_t pid = -1;
static pid_t child = -1;

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
  #ifndef event_match

  #ifdef PAPI_API_DEBUG

  #define event_match(event_name, string, event_id, tfunc, type)  \
    if(strncmp(event_name, string, strlen(string)) == 0) {        \
      if(PAPI_query_event(event_id) != PAPI_OK) {                 \
        fprintf(stderr, "Event not available: " #event_id "\n");  \
      }                                                           \
                                                                  \
      return new_event(event_id, tfunc, type);                    \
    }

  #else

  #define event_match(event_name, string, event_id, tfunc, type)  \
    if(strncmp(event_name, string, strlen(string)) == 0) {        \
      return new_event(event_id, tfunc, type);                    \
    }

  #endif

  /* Instructions */
  event_match(event_name, "PAPI_TOT_CYC", PAPI_TOT_CYC, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_TOT_INS", PAPI_TOT_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_VEC_INS", PAPI_VEC_INS, transform_skip, VALUE_TYPE_INTEGER);

  /* Memory */
  event_match(event_name, "PAPI_L1_DCA", PAPI_L1_DCA, transform_skip, VALUE_TYPE_TWO_INTEGERS);
  event_match(event_name, "PAPI_L1_DCM", PAPI_L1_DCM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_DCH", PAPI_L1_DCH, transform_skip, VALUE_TYPE_FLOAT);
  event_match(event_name, "PAPI_L2_DCA", PAPI_L2_DCA, transform_skip, VALUE_TYPE_TWO_INTEGERS);
  event_match(event_name, "PAPI_L2_DCM", PAPI_L2_DCM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_DCH", PAPI_L2_DCH, transform_skip, VALUE_TYPE_FLOAT);

  #endif
  #undef event_match

  return NULL;
}

struct papi_api_config *get_papi_api_config() {
  FILE *fp;
  struct papi_api_config *config;
  char config_line[MAX_CONFIG_LINE];
  char *line = NULL;
  size_t len, i, j, eq;
  ssize_t read;

  if((fp = fopen(PERF_HALIDE_CONFIG_FILE, "r")) == NULL) {
    fprintf(stderr, "Configuration file for perf_halide not found: \"%s\"\n", PERF_HALIDE_CONFIG_FILE);
    return NULL;
  }

  config = (struct papi_api_config *) malloc(sizeof(struct papi_api_config));

  if(config != NULL) {
    while((read = getline(&line, &len, fp)) > 0) {
      for(i = 0, j = 0, eq = -1; i < len; ++i) {
        if(line[i] != ' ') {
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

void print_event_values(int func, long long int *values, struct papi_api_event *events) {
  struct papi_api_event *event;
  long long int tvalue, tvalue2;
  unsigned int i;

  fprintf(stdout, "%d: ", func);

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

  fprintf(stdout, "\n");
}

int papi_halide_initialize() {
  int ret;

  global_state = (struct papi_api_state *) malloc(sizeof(struct papi_api_state));

  if(global_state == NULL) {
    fprintf(stderr, "papi_marker_start(): Failed to allocate global state!\n");
    return -1;
  }

  global_state->papi_started = 0;
  global_state->event_set = -1;
  global_state->events = NULL;

  global_config = get_papi_api_config();
  global_state->num_events = build_events(&(global_state->events), event_array, global_config);

  if((ret = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
    fprintf(stderr, "PAPI_library_init(): %d", ret);
    return -1;
  }

  if((ret = PAPI_create_eventset(&(global_state->event_set))) != PAPI_OK) {
    fprintf(stderr, "PAPI_create_eventset(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_add_events(global_state->event_set, event_array, global_state->num_events)) != PAPI_OK) {
    fprintf(stderr, "PAPI_add_events(): %d\n", ret);
    return -1;
  }

  return 0;
}

int papi_halide_marker_start(int func) {
  int ret;

  if((ret = PAPI_start(global_state->event_set)) != PAPI_OK) {
    fprintf(stderr, "PAPI_start(): %d\n", ret);
    return -1;
  }

  global_state->papi_started = 1;
  return 0;
}

int papi_halide_marker_stop(int func) {
  long long int values[MAX_PAPI_EVENTS];
  long long int dummyvalues[MAX_PAPI_EVENTS];
  int ret;

  if((ret = PAPI_stop(global_state->event_set, dummyvalues)) != PAPI_OK) {
    fprintf(stderr, "PAPI_stop(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_read(global_state->event_set, values)) != PAPI_OK) {
    fprintf(stderr, "PAPI_read(): %d\n", ret);
    return -1;
  }

  print_event_values(func, values, global_state->events);
  global_state->papi_started = 0;
  return 0;
}

int papi_halide_marker_start_child(int func) {
  int ret, status, sig;

  pid = fork();

  if(pid < 0) {
    fprintf(stderr, "fork(): Failed to create a child process!\n");
    return -1;
  }

  /* This is the process that will execute the profiler */
  if(pid != 0) {
    if((ret = PAPI_assign_eventset_component(global_state->event_set, 0)) != PAPI_OK) {
      fprintf(stderr, "PAPI_assign_eventset_component(): %d\n", ret);
    }

    if((ret = PAPI_attach(global_state->event_set, (unsigned long) pid)) != PAPI_OK) {
      fprintf(stderr, "PAPI_attach(): %d\n", ret);
    }

    if((ret = PAPI_start(global_state->event_set)) != PAPI_OK) {
      fprintf(stderr, "PAPI_start(): %d\n", ret);
    }

    child = wait(&status);

    #ifdef PAPI_API_DEBUG

    if(WIFSTOPPED(status)) {
      sig = WSTOPSIG(status);
      fprintf(stderr, "Child has stopped due to signal %d (%s)\n", sig, strsignal(sig));
    }

    if(WIFSIGNALED(status)) {
      sig = WTERMSIG(status);
      fprintf(stderr, "Child %ld received signal %d (%s)\n", (long) child, sig, strsignal(sig));
    }

    #else

    (void)sig;

    #endif

    if((ret = ptrace(PTRACE_CONT, pid, NULL, NULL)) < 0) {
      fprintf(stderr, "ptrace(): Failed to continue child process (%d)\n", ret);
    }

    do {
      child = wait(&status);

      #ifdef PAPI_API_DEBUG

      if(WIFSTOPPED(status)) {
        sig = WSTOPSIG(status);
        fprintf(stderr, "Child has stopped due to signal %d (%s)\n", sig, strsignal(sig));
      }

      if(WIFSIGNALED(status)) {
        sig = WTERMSIG(status);
        fprintf(stderr, "Child %ld received signal %d (%s)\n", (long) child, sig, strsignal(sig));
      }

      #endif

    } while(!WIFEXITED(status));

  /* This is the process that will execute the Halide program */
  } else {
    if(ptrace(PTRACE_TRACEME) == 0) {
      raise(SIGSTOP);
    }
  }

  return pid;
}

int papi_halide_marker_stop_child(int func) {
  long long int values[MAX_PAPI_EVENTS];
  int ret;

  if(pid != 0) {
    if((ret = PAPI_read(global_state->event_set, values)) != PAPI_OK) {
      fprintf(stderr, "PAPI_read(): %d\n", ret);
    } else {
      print_event_values(func, values, global_state->events);
    }
  }

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
  PAPI_shutdown();
}
