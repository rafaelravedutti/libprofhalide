#ifndef __PAPI_HALIDE_H__
#define __PAPI_HALIDE_H__

# ifndef PAPI_VERBOSE
#   define PAPI_PRINT
# else
#   define PAPI_PRINT           printf
# endif

#define _papi_stringify(x) #x
#define _papi_expand_and_stringify(x) _perf_stringify(x)
#define papi_assert(cond)                                                                               \
  if(!(cond)) {                                                                                         \
    fprintf(stderr, __FILE__ ":" _papi_expand_and_stringify(__LINE__) " Assert failed: " #cond "\n");   \
    exit(-1);                                                                                           \
  }

#define MAX_PAPI_EVENTS         128
#define MAX_PAPI_DESCRIPTORS    128

//extern "C" {

extern int papi_halide_initialize();
extern int papi_halide_marker_start(int func);
extern int papi_halide_marker_stop(int func, long long int *values);
extern int papi_halide_marker_start_child(int func);
extern int papi_halide_marker_stop_child(int func, long long int *values);
extern int papi_halide_number_of_events();
extern void papi_halide_shutdown();

//}

#endif
