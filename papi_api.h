#include "papi_halide.h"

#define PAPI_HALIDE_CONFIG_FILE   "papi_halide_events.conf"
#define MAX_CONFIG_LINE           1024

#define VALUE_TYPE_INTEGER        0
#define VALUE_TYPE_FLOAT          1
#define VALUE_TYPE_TWO_INTEGERS   2

struct papi_api_config {
  char events[MAX_CONFIG_LINE];
};

struct papi_api_event {
  int event_id;
  int event_value_type;
  long long int (*event_transform)(long long int);
  struct papi_api_event *next;
};

struct papi_api_state {
  int papi_meas[MAX_PAPI_DESCRIPTORS];
  int event_array[MAX_PAPI_EVENTS];
  int event_set;
  int num_events;
  struct papi_api_event *events;
};
