# PAPI directory
PAPI_PATH=/home/hpc/iwia/iwia021h/papi

# Likwid directory
LIKWID_PATH=$(shell dirname $(shell which likwid-perfctr))
#LIKWID_PATH=/mnt/opt/likwid-5.1.0
#LIKWID_PATH=/apps/likwid/5.1.0a-msr

# C compiler and flags
CC=gcc
CFLAGS=-DPERF_VERBOSE -Wall

all: libpapihalide.so liblikwidhalide.so libtimehalide.so

likwid_api.o: likwid_api.c
	${CC} -c -fPIC $^ -o $@ ${CFLAGS} -I ${LIKWID_PATH}/../include -DLIKWID_PERFMON

papi_api.o: papi_api.c
	${CC} -c -fPIC $^ -o $@ ${CFLAGS} -I ${PAPI_PATH}/src

time_api.o: time_api.c
	${CC} -c -fPIC $^ -o $@ ${CFLAGS}

libpapihalide.so: papi_api.o
	${CC} -shared -fPIC $^ -lc -o $@

liblikwidhalide.so: likwid_api.o
	${CC} -shared -fPIC $^ -lc -o $@

libtimehalide.so: time_api.o
	${CC} -shared -fPIC $^ -lc -o $@

install:
	cp -p libpapihalide.so liblikwidhalide.so libtimehalide.so /usr/lib

clean:
	rm -f libpapihalide.so liblikwidhalide.so libtimehalide.so *.o
