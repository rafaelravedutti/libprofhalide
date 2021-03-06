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
#include "perfctr_halide.h"

#define NUM_MARKER_TYPES        4
#define MAX_MARKERS_PER_TYPE    128
#define MAX_THREADS             32
#define MAX_EVENTS_PER_THREAD   16

#define PAPI_HALIDE_CONFIG_FILE   "perfctr_halide_events.conf"
#define MAX_CONFIG_LINE           1024

struct papi_api_config {
    char events[MAX_CONFIG_LINE];
};

struct papi_api_event {
    int event_id;
    struct papi_api_event *next;
};

struct papi_api_state {
    int event_array[MAX_EVENTS_PER_THREAD];
    int event_set;
    int num_events;
    struct papi_api_event *events;
};


static struct papi_api_config *global_config = NULL;
static struct papi_api_state *global_state = NULL;

pthread_mutex_t lock;

struct threadInfo {
    int event_set;
    int thread_id;
    int set;
    int level;
};

static long long counters[MAX_MARKERS_PER_TYPE][NUM_MARKER_TYPES][MAX_THREADS][MAX_EVENTS_PER_THREAD];
static bool counters_set_flag[MAX_MARKERS_PER_TYPE][NUM_MARKER_TYPES][MAX_THREADS];
static struct threadInfo threadsInfo[MAX_THREADS];

int find_or_insert_thread(int *inserted) {
    int thid = pthread_self();
    int first_available_idx = -1;

    for(int i = 0; i < MAX_THREADS; ++i) {
        if(threadsInfo[i].set && threadsInfo[i].thread_id == thid) {
            *inserted = 0;
            return i;
        }
    }

    pthread_mutex_lock(&lock);
    for(int i = 0; i < MAX_THREADS; ++i) {
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
    threadsInfo[first_available_idx].set = 1;
    pthread_mutex_unlock(&lock);
    *inserted = 1;
    return first_available_idx;
}

struct papi_api_event *new_event(int event_id) {
    struct papi_api_event *event = (struct papi_api_event *) malloc(sizeof(struct papi_api_event));
    if(event == NULL) {
        fprintf(stderr, "new_event(): Failed to allocate papi_api_event!\n");
        return NULL;
    }

    event->event_id = event_id;
    event->next = NULL;
    return event;
}

struct papi_api_event *get_event(const char *event_name) {
    int event_id, ret;
    if((ret = PAPI_event_name_to_code(event_name, &event_id)) != PAPI_OK) {
        fprintf(stderr, "PAPI_event_name_to_code(%s): %d\n", event_name, ret);
        return NULL;
    }

    return new_event(event_id);
}

struct papi_api_config *get_papi_api_config() {
    FILE *fp;
    struct papi_api_config *config;
    char config_line[MAX_CONFIG_LINE];
    char *line = NULL;
    size_t len, i, j, eq;
    ssize_t read;

    if((fp = fopen(PAPI_HALIDE_CONFIG_FILE, "r")) == NULL) {
        fprintf(stderr, "Configuration file for perfctr_halide not found: \"%s\"\n", PAPI_HALIDE_CONFIG_FILE);
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

int perfctr_halide_initialize() {
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

    global_state->event_set = -1;
    global_state->events = NULL;
    global_config = get_papi_api_config();
    global_state->num_events = build_events(&(global_state->events), global_state->event_array, global_config);

    for(int t = 0; t < NUM_MARKER_TYPES; t++) {
        for(int m = 0; m < MAX_MARKERS_PER_TYPE; m++) {
            for(int th = 0; th < MAX_MARKERS_PER_TYPE; th++) {
                counters_set_flag[t][m][th] = false;
            }
        }
    }

    if((ret = PAPI_thread_init((unsigned long (*)(void))(pthread_self))) != PAPI_OK) {
        fprintf(stderr, "PAPI_thread_init(): %d\n", ret);
        return -1;
    }

    return 0;
}

int start_thread() {
    int ret;
    int inserted;
    int thread_idx = find_or_insert_thread(&inserted);

    if((ret = PAPI_register_thread()) != PAPI_OK) {
        fprintf(stderr, "PAPI_register_thread(): %d\n", ret);
        return -1;
    }

    if((ret = PAPI_create_eventset(&threadsInfo[thread_idx].event_set)) != PAPI_OK) {
        fprintf(stderr, "PAPI_create_eventset(): %d\n", ret);
        return -1;
    }

    for(int i = 0; i < global_state->num_events; ++i) {
        if((ret = PAPI_add_event(threadsInfo[thread_idx].event_set, global_state->event_array[i])) != PAPI_OK) {
            fprintf(stderr, "PAPI_add_event(%d): %d\n", global_state->event_array[i], ret);
            return -1;
        }
    }

    if((ret = PAPI_start(threadsInfo[thread_idx].event_set)) != PAPI_OK) {
        fprintf(stderr, "PAPI_start(): %d\n", ret);
        return -1;
    }

    return thread_idx;
}

int stop_thread(int thread_idx) {
    long long int dummyvalues[MAX_EVENTS_PER_THREAD];
    int ret;

    if((ret = PAPI_stop(threadsInfo[thread_idx].event_set, dummyvalues)) != PAPI_OK) {
        fprintf(stderr, "PAPI_stop(): %d\n", ret);
        return -1;
    }

    for(int i = 0; i < global_state->num_events; ++i) {
        if((ret = PAPI_remove_event(threadsInfo[thread_idx].event_set, global_state->event_array[i])) != PAPI_OK) {
            fprintf(stderr, "PAPI_remove_event(%d): %d\n", global_state->event_array[i], ret);
            return -1;
        }
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

int perfctr_halide_marker_register(const char *marker, int id, int type) {
    return 0;
}

int perfctr_halide_marker_start(const char *marker, int id, int type) {
    int thread_idx = start_thread();
    PAPI_reset(threadsInfo[thread_idx].event_set);
    return 0;
}

int perfctr_halide_marker_stop(const char *marker, int id, int type) {
    int inserted;
    int thread_idx = find_or_insert_thread(&inserted);
    PAPI_accum(threadsInfo[thread_idx].event_set, (long long *)(&counters[id][type][thread_idx]));
    counters_set_flag[id][type][thread_idx] = true;
    stop_thread(thread_idx);
    return 0;
}

void perfctr_halide_shutdown() {
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
