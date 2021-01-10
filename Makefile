# C compiler
CC=gcc
# C compilation flags
CFLAGS=-DPERF_VERBOSE -Wall -I ${PAPI_SOURCE_DIR}

# Current directory
CURRENT_DIR := ${CURDIR}

# Halide directories
HALIDE_SRC=/home/vault/iwia/iwia021h/Halide
HALIDE_RUNTIME_DIR=${HALIDE_DIR}/src/runtime

# Halide flags
HALIDE_FLAGS=-I ${HALIDE_RUNTIME_DIR}

# PAPI directory
PAPI_DIR=/home/hpc/iwia/iwia021h/papi
PAPI_SOURCE_DIR=${PAPI_DIR}/src

# Likwid directory
LIKWID_DIR=/mnt/opt/likwid-5.1.0

# Halide files to synchronize
HALIDE_SYNC_FILES = \
	src/CodeGen_Internal.cpp \
	src/Func.cpp \
	src/Func.h \
	src/Function.cpp \
	src/Function.h \
	src/LLVM_Runtime_Linker.cpp \
	src/Lower.cpp \
	src/Pipeline.cpp \
	src/Target.cpp \
	src/Target.h \
	src/runtime/HalideRuntime.h \
	src/runtime/runtime_api.cpp \
	src/runtime/runtime_internal.h

all: libpapihalide.so liblikwidhalide.so

transform.o: transform.c
	${CC} -c -fPIC $^ -o $@ ${CFLAGS}

likwid_api.o: likwid_api.c
	${CC} -c -fPIC $^ -o $@ ${CFLAGS} -I ${LIKWID_DIR}/include -DLIKWID_PERFMON

papi_api.o: papi_api.c
	${CC} -c -fPIC $^ -o $@ ${CFLAGS}

perf_api.o: perf_api.c
	${CC} -c -fPIC $^ -o $@ ${CFLAGS}

libpapihalide.so: papi_api.o perf_api.o transform.o
	${CC} -shared -fPIC $^ -lc -o $@

liblikwidhalide.so: likwid_api.o
	${CC} -shared -fPIC $^ -lc -o $@

install:
	cp -p libpapihalide.so liblikwidhalide.so /usr/lib

test: test.c libpapihalide.so
	${CC} $^ -L. -lpapihalide -lpapi -o $@

clean:
	rm -f libpapihalide.so liblikwidhalide.so *.o test

sync_halide:
	cp -p ${HALIDE_DIR}/Makefile halide/Makefile
	cd ${HALIDE_DIR} && git diff Makefile > ${CURRENT_DIR}/halide/Makefile.diff && cd - > /dev/null
	for file in ${HALIDE_SYNC_FILES}; do \
		cp -p ${HALIDE_DIR}/$${file} halide/$${file%/*}/modified/$${file##*/}; \
		cd ${HALIDE_DIR} && \
		git diff $${file} > ${CURRENT_DIR}/halide/$${file%/*}/modified/$${file##*/}.diff && \
		cd - > /dev/null; \
	done
