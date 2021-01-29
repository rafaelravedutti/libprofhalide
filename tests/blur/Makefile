# C and C++ compilers
CC=gcc
CXX=g++

# C and C++ compilers flags
CCFLAGS=
CXXFLAGS=-g

# Image sizes 3840x2160 (4K), 10240x4320 (10K), 10112x10112
# Channels are usually 1 or 3
IMAGE_WIDTH=10240
IMAGE_HEIGHT=4320
IMAGE_CHANNELS=3

# Include image settings in flags
CXXFLAGS+=-DSCHEDULE=${SCHEDULE} -DIMAGE_WIDTH=${IMAGE_WIDTH} -DIMAGE_HEIGHT=${IMAGE_HEIGHT} -DIMAGE_CHANNELS=${IMAGE_CHANNELS}

# Must profile code?
ifdef PROFILE
	CXXFLAGS+=-DPROFILE
endif

# Must use parallel schedule?
ifdef PARALLEL
	CXXFLAGS+=-DPARALLEL
endif

# Halide path
HALIDE_PATH=/home/vault/iwia/iwia021h/Halide

# Use Halide static library?
USE_HALIDE_STATIC_LIB=false

# PAPI Halide path
PROF_HALIDE_PATH=../..
#PROF_HALIDE_PATH=/home/hpc/iwia/iwia021h/libprofhalide

# Halide flags
HALIDE_FLAGS=-I ${HALIDE_PATH}/include -I ${HALIDE_PATH}/tools -L ${HALIDE_PATH}/bin -L ${PROF_HALIDE_PATH} -llikwidhalide -lpthread -lz -ldl -std=c++11
#HALIDE_FLAGS=-I ${HALIDE_PATH}/include -I ${HALIDE_PATH}/tools -L ${HALIDE_PATH}/bin -L ${PROF_HALIDE_PATH} -lpapihalide -lpthread -lz -ldl -std=c++11
#HALIDE_FLAGS=-I ${HALIDE_PATH}/include -I ${HALIDE_PATH}/tools -L ${HALIDE_PATH}/bin -L ${PROF_HALIDE_PATH} -lpapihalide -lpthread -ljpeg -lz `libpng-config --cflags --ldflags` -ldl -std=c++11

ifeq (${USE_HALIDE_STATIC_LIB}, true)
	HALIDE_FLAGS+=${HALIDE_PATH}/lib/libHalide.a 
else
	HALIDE_FLAGS+=-L ${HALIDE_PATH}/bin -lHalide
endif

# PAPI path
PAPI_PATH=/home/hpc/iwia/iwia021h/papi
PAPI_SRC=${PAPI_PATH}/src

# PAPI flags
PAPI_FLAGS=-L ${PAPI_SRC} -lpapi

# PFM path
LIBPFM_PATH=${PAPI_SRC}/libpfm4/lib/

# Likwid path
#LIKWID_PATH=$(dirname $(which likwid-perfctr))
LIKWID_PATH=/mnt/opt/likwid-5.1.0/bin
LIKWID_LIB=${LIKWID_PATH}/../lib
LIKWID_INCLUDE=${LIKWID_PATH}/../include

all: blur_jit blur_aot

blur_jit: blur.cpp
	${CXX} $^ ${CXXFLAGS} ${HALIDE_FLAGS} -L ${LIKWID_LIB} -llikwid -o $@

blur_gen_aot: blur.cpp
	${CXX} $^ ${CXXFLAGS} ${HALIDE_FLAGS} -L ${LIKWID_LIB} -llikwid -DCOMPILE_AOT -o $@
	#${CXX} $^ ${CXXFLAGS} ${HALIDE_FLAGS} ${PAPI_FLAGS} -DCOMPILE_AOT -o $@

blur_aot.cpp: blur_gen_aot
	./$^

blur_aot: blur_aot.cpp blur_aot.h blur_aot.a
	${CXX} $^ ${CXXFLAGS} ${HALIDE_FLAGS} -L ${LIKWID_LIB} -llikwid ${PAPI_FLAGS} -L ${LIBPFM_PATH} -lpfm -o $@

likwid_test: likwid_test.c
	${CC} -fopenmp -DLIKWID_PERFMON -L ${LIKWID_LIB} -I ${LIKWID_INCLUDE} $^ -o $@ -llikwid

perf_test: perf_example.c
	${CC} $^ -o $@

clean:
	rm -f blur_jit blur_gen_aot blur_aot.h blur_aot.a blur_aot likwid_test perf_test
