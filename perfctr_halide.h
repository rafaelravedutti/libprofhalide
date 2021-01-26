#ifndef __PERFCTR_HALIDE_H__
#define __PERFCTR_HALIDE_H__

//extern "C" {

/* Initialization and markers */
extern int perfctr_halide_initialize();
extern int perfctr_halide_marker_register(const char *marker, int id, int type);
extern int perfctr_halide_marker_start(const char *marker, int id, int type);
extern int perfctr_halide_marker_stop(const char *marker, int id, int type);
extern void perfctr_halide_shutdown();

//}

#endif
