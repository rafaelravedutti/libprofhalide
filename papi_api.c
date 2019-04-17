#include <papi.h>
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

static pid_t pid = -1;
static pid_t child = -1;

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

  event_match(event_name, "PAPI_L1_DCM", PAPI_L1_DCM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_ICM", PAPI_L1_ICM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_DCM", PAPI_L2_DCM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_ICM", PAPI_L2_ICM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_DCM", PAPI_L3_DCM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_ICM", PAPI_L3_ICM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_TCM", PAPI_L1_TCM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_TCM", PAPI_L2_TCM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_TCM", PAPI_L3_TCM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_CA_SNP", PAPI_CA_SNP, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_CA_SHR", PAPI_CA_SHR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_CA_CLN", PAPI_CA_CLN, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_CA_INV", PAPI_CA_INV, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_CA_ITV", PAPI_CA_ITV, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_LDM", PAPI_L3_LDM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_STM", PAPI_L3_STM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BRU_IDL", PAPI_BRU_IDL, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FXU_IDL", PAPI_FXU_IDL, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FPU_IDL", PAPI_FPU_IDL, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_LSU_IDL", PAPI_LSU_IDL, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_TLB_DM", PAPI_TLB_DM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_TLB_IM", PAPI_TLB_IM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_TLB_TL", PAPI_TLB_TL, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_LDM", PAPI_L1_LDM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_STM", PAPI_L1_STM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_LDM", PAPI_L2_LDM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_STM", PAPI_L2_STM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BTAC_M", PAPI_BTAC_M, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_PRF_DM", PAPI_PRF_DM, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_DCH", PAPI_L3_DCH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_TLB_SD", PAPI_TLB_SD, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_CSR_FAL", PAPI_CSR_FAL, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_CSR_SUC", PAPI_CSR_SUC, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_CSR_TOT", PAPI_CSR_TOT, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_MEM_SCY", PAPI_MEM_SCY, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_MEM_RCY", PAPI_MEM_RCY, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_MEM_WCY", PAPI_MEM_WCY, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_STL_ICY", PAPI_STL_ICY, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FUL_ICY", PAPI_FUL_ICY, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_STL_CCY", PAPI_STL_CCY, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FUL_CCY", PAPI_FUL_CCY, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_HW_INT", PAPI_HW_INT, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BR_UCN", PAPI_BR_UCN, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BR_CN", PAPI_BR_CN, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BR_TKN", PAPI_BR_TKN, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BR_NTK", PAPI_BR_NTK, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BR_MSP", PAPI_BR_MSP, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BR_PRC", PAPI_BR_PRC, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FMA_INS", PAPI_FMA_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_TOT_IIS", PAPI_TOT_IIS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_TOT_INS", PAPI_TOT_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_INT_INS", PAPI_INT_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FP_INS", PAPI_FP_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_LD_INS", PAPI_LD_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_SR_INS", PAPI_SR_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_BR_INS", PAPI_BR_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_VEC_INS", PAPI_VEC_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_RES_STL", PAPI_RES_STL, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FP_STAL", PAPI_FP_STAL, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_TOT_CYC", PAPI_TOT_CYC, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_LST_INS", PAPI_LST_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_SYC_INS", PAPI_SYC_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_DCH", PAPI_L1_DCH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_DCH", PAPI_L2_DCH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_DCA", PAPI_L1_DCA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_DCA", PAPI_L2_DCA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_DCA", PAPI_L3_DCA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_DCR", PAPI_L1_DCR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_DCR", PAPI_L2_DCR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_DCR", PAPI_L3_DCR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_DCW", PAPI_L1_DCW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_DCW", PAPI_L2_DCW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_DCW", PAPI_L3_DCW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_ICH", PAPI_L1_ICH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_ICH", PAPI_L2_ICH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_ICH", PAPI_L3_ICH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_ICA", PAPI_L1_ICA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_ICA", PAPI_L2_ICA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_ICA", PAPI_L3_ICA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_ICR", PAPI_L1_ICR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_ICR", PAPI_L2_ICR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_ICR", PAPI_L3_ICR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_ICW", PAPI_L1_ICW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_ICW", PAPI_L2_ICW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_ICW", PAPI_L3_ICW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_TCH", PAPI_L1_TCH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_TCH", PAPI_L2_TCH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_TCH", PAPI_L3_TCH, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_TCA", PAPI_L1_TCA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_TCA", PAPI_L2_TCA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_TCA", PAPI_L3_TCA, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_TCR", PAPI_L1_TCR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_TCR", PAPI_L2_TCR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_TCR", PAPI_L3_TCR, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L1_TCW", PAPI_L1_TCW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L2_TCW", PAPI_L2_TCW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_L3_TCW", PAPI_L3_TCW, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FML_INS", PAPI_FML_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FAD_INS", PAPI_FAD_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FDV_INS", PAPI_FDV_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FSQ_INS", PAPI_FSQ_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FNV_INS", PAPI_FNV_INS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_FP_OPS", PAPI_FP_OPS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_SP_OPS", PAPI_SP_OPS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_DP_OPS", PAPI_DP_OPS, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_VEC_SP", PAPI_VEC_SP, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_VEC_DP", PAPI_VEC_DP, transform_skip, VALUE_TYPE_INTEGER);
  event_match(event_name, "PAPI_REF_CYC", PAPI_REF_CYC, transform_skip, VALUE_TYPE_INTEGER);

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

  if((fp = fopen(PAPI_HALIDE_CONFIG_FILE, "r")) == NULL) {
    fprintf(stderr, "Configuration file for papi_halide not found: \"%s\"\n", PAPI_HALIDE_CONFIG_FILE);
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

  global_state = (struct papi_api_state *) malloc(sizeof(struct papi_api_state));

  if(global_state == NULL) {
    fprintf(stderr, "papi_marker_start(): Failed to allocate global state!\n");
    return -1;
  }

  global_state->event_set = -1;
  global_state->events = NULL;

  global_config = get_papi_api_config();
  global_state->num_events = build_events(&(global_state->events), global_state->event_array, global_config);

  for(int i = 0; i < MAX_PAPI_DESCRIPTORS; ++i) {
    global_state->papi_meas[i] = 0;
  }

  if((ret = PAPI_library_init(PAPI_VER_CURRENT)) != PAPI_VER_CURRENT) {
    fprintf(stderr, "PAPI_library_init(): %d", ret);
    return -1;
  }

  if((ret = PAPI_create_eventset(&(global_state->event_set))) != PAPI_OK) {
    fprintf(stderr, "PAPI_create_eventset(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_add_events(global_state->event_set, global_state->event_array, global_state->num_events)) != PAPI_OK) {
    fprintf(stderr, "PAPI_add_events(): %d\n", ret);
    return -1;
  }

  if((ret = PAPI_start(global_state->event_set)) != PAPI_OK) {
    fprintf(stderr, "PAPI_start(): %d\n", ret);
    return -1;
  }

  return 0;
}

int papi_halide_number_of_events() {
  return global_state->num_events;
}

int papi_halide_marker_start(int func) {
  int ret;

  if((ret = PAPI_reset(global_state->event_set)) != PAPI_OK) {
    fprintf(stderr, "PAPI_reset(): %d\n", ret);
    return -1;
  }

  global_state->papi_meas[func]++;
  return 0;
}

int papi_halide_marker_stop(int func, long long int *values) {
  int ret;

  if(global_state->papi_meas[func] <= 1) {
    if((ret = PAPI_read(global_state->event_set, values)) != PAPI_OK) {
      fprintf(stderr, "PAPI_read(): %d\n", ret);
      return -1;
    }
  } else {
    if((ret = PAPI_accum(global_state->event_set, values)) != PAPI_OK) {
      fprintf(stderr, "PAPI_accum(): %d\n", ret);
      return -1;
    }
  }

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

int papi_halide_marker_stop_child(int func, long long int *values) {
  int ret;

  if(pid != 0) {
    if((ret = PAPI_read(global_state->event_set, values)) != PAPI_OK) {
      fprintf(stderr, "PAPI_read(): %d\n", ret);
      return -1;
    }
  }

  return 0;
}

void papi_halide_shutdown() {
  long long int dummyvalues[MAX_PAPI_EVENTS];
  struct papi_api_event *event, *previous;
  int ret;

  if((ret = PAPI_stop(global_state->event_set, dummyvalues)) != PAPI_OK) {
    fprintf(stderr, "PAPI_stop(): %d\n", ret);
    return;
  }

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
