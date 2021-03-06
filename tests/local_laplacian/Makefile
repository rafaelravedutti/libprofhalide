# C and C++ compilers
CC=gcc
CXX=g++

# C and C++ compilers flags
CCFLAGS=
CXXFLAGS=-g

# Must profile code?
ifdef PROFILE
	CXXFLAGS+=-DPROFILE
endif

# Must use parallel schedule?
ifdef PARALLEL
	CXXFLAGS+=-DPARALLEL
endif

# Must use schedule with vectorization?
ifdef VECTORIZE
	CXXFLAGS+=-DVECTORIZE
endif

# Halide path
HALIDE_PATH=/home/ppginf/rrlmachado/nobackup/repositories/Halide

# Use Halide static library?
USE_HALIDE_STATIC_LIB=false

# Halide flags
HALIDE_FLAGS=-I ${HALIDE_PATH}/include -I ${HALIDE_PATH}/tools -L ${HALIDE_PATH}/bin -L ${PAPI_HALIDE_PATH} -lpapihalide -lpthread -ljpeg -lz `libpng-config --cflags --ldflags` -ldl -std=c++11

ifeq (${USE_HALIDE_STATIC_LIB}, true)
	HALIDE_FLAGS+=${HALIDE_PATH}/lib/libHalide.a 
else
	HALIDE_FLAGS+=-L ${HALIDE_PATH}/bin -lHalide
endif

# PAPI path
PAPI_PATH=/home/ppginf/rrlmachado/nobackup/repositories/papi
PAPI_SRC=${PAPI_PATH}/src

# PAPI flags
PAPI_FLAGS=-L ${PAPI_SRC} -lpapi

# PFM path
LIBPFM_PATH=${PAPI_SRC}/libpfm4/lib/

# PAPI Halide path
PAPI_HALIDE_PATH=/home/ppginf/rrlmachado/nobackup/repositories/libpapihalide

# Likwid path
LIKWID_PATH=$(dirname $(which likwid-perfctr))
LIKWID_LIB=${LIKWID_PATH}/../lib
LIKWID_INCLUDE=${LIKWID_PATH}/../include

all: local_laplacian_jit local_laplacian_aot

local_laplacian_jit: local_laplacian.cpp
	${CXX} $^ ${CXXFLAGS} ${HALIDE_FLAGS} ${PAPI_FLAGS} -DSCHEDULE=${SCHEDULE} -o $@

local_laplacian_gen_aot: local_laplacian.cpp
	${CXX} $^ ${CXXFLAGS} ${HALIDE_FLAGS} ${PAPI_FLAGS} -DSCHEDULE=${SCHEDULE} -DCOMPILE_AOT -o $@

local_laplacian_aot.cpp: local_laplacian_gen_aot
	./$^

local_laplacian_aot: local_laplacian_aot.cpp local_laplacian_aot.h local_laplacian_aot.a
	${CXX} $^ ${CXXFLAGS} ${HALIDE_FLAGS} ${PAPI_FLAGS} -L ${LIBPFM_PATH} -lpfm -o $@

likwid_test: likwid_test.c
	${CC} -fopenmp -DLIKWID_PERFMON -L ${LIKWID_LIB} -I ${LIKWID_INCLUDE} $^ -o $@ -llikwid

perf_test: perf_example.c
	${CC} $^ -o $@

clean:
	rm -f local_laplacian_jit local_laplacian_gen_aot local_laplacian_aot.h local_laplacian_aot.a local_laplacian_aot likwid_test perf_test
