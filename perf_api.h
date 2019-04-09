#ifndef __PERF_HALIDE_H__
#define __PERF_HALIDE_H__

# ifndef PERF_VERBOSE
#   define PERF_PRINT
# else
#   define PERF_PRINT           printf
# endif

#define _perf_stringify(x) #x
#define _perf_expand_and_stringify(x) _perf_stringify(x)
#define perf_assert(cond)                                                                               \
  if(!(cond)) {                                                                                         \
    fprintf(stderr, __FILE__ ":" _perf_expand_and_stringify(__LINE__) " Assert failed: " #cond "\n");   \
    exit(-1);                                                                                           \
  }

#define MAX_PERF_DESCRIPTORS    128

//extern "C" {

extern int get_perf_descriptor(int marker);
extern int perf_descriptor_start(int marker);
extern int perf_descriptor_stop(int marker);

//}

#endif
