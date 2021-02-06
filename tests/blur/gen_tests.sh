#!/bin/bash

# Algorithm
ALGORITHM="blur"

# Schedules
SCHEDULES="breadth_first full_fusion sliding_window tile_32x32"

# Target architecture
ARCH="host"
#ARCH="host-x86-64" # no vectorization

# Number of threads and pinning string on parallel schedules
NTHREADS=24
#PIN_STRING="S0:0-5"
#PIN_STRING="M0:0-5"
PIN_STRING="M0:23,0-22"
#PIN_STRING="M0"

# Group (Likwid)
GROUP=MEM

# Image sizes 3840x2160 (4K), 10240x4320 (10K), 10112x10112
# Channels are usually 1 or 3
#IMAGE_WIDTH=10112
#IMAGE_HEIGHT=10112
IMAGE_WIDTH=10240
IMAGE_HEIGHT=4320
IMAGE_CHANNELS=3
IMAGE_SIZE="${IMAGE_WIDTH}x${IMAGE_HEIGHT}x${IMAGE_CHANNELS}"
IMAGE_SIZE_PARAMS="IMAGE_WIDTH=${IMAGE_WIDTH} IMAGE_HEIGHT=${IMAGE_HEIGHT} IMAGE_CHANNELS=${IMAGE_CHANNELS}"

# Run serial and/or parallel schedules
RUN_SERIAL=0
RUN_PARALLEL=1

# Measure events and/or time
MEASURE_EVENTS=1
MEASURE_TIME=0

# Treated hostname (specific to RRZE test and woody clusters)
TREATED_HOST=$(hostname | sed -E "s/tg09[0-4]/rome/" | sed "s/medusa/cascadelake/")

# Retrieve library paths
source source.me

# Define and create directory structure for results
DIR_PREFIX="results/${TREATED_HOST}/${ALGORITHM}/${IMAGE_SIZE}"
mkdir -p ${DIR_PREFIX}/${GROUP}
mkdir -p ${DIR_PREFIX}/TIME

# Profiler tests
if [ "${MEASURE_EVENTS}" -ne "0" ]; then
    # Serial schedules
    sched_id=1
    for sched in ${SCHEDULES}; do
        if [ "${RUN_SERIAL}" -ne "0" ]; then
            export HL_TARGET="${ARCH}-perfctr"
            export HL_JIT_TARGET="${ARCH}-perfctr"
            make clean && make SCHEDULE=${sched_id} PROFILE=y ${IMAGE_SIZE_PARAMS}
            rm -f ${DIR_PREFIX}/${GROUP}/${sched}_serial.txt
            echo "Running profiler tests for serial ${sched} schedule..."
            for i in $(seq 1 3); do
                likwid-perfctr -C 0 -g ${GROUP} -m ./blur_aot | tee -a ${DIR_PREFIX}/${GROUP}/${sched}_serial.txt ;
            done
        fi

        sched_id=$((sched_id + 1))
    done


    # Parallel schedules
    sched_id=1
    for sched in ${SCHEDULES}; do
        if [ "${RUN_PARALLEL}" -ne "0" -a "$sched_id" -ne "3" ]; then
            export HL_TARGET="${ARCH}-perfctr"
            export HL_JIT_TARGET="${ARCH}-perfctr"
            export HL_NUM_THREADS=${NTHREADS}
            #export OMP_NUM_THREADS="${HL_NUM_THREADS}"
            make clean && make SCHEDULE=${sched_id} PARALLEL=y PROFILE=y ${IMAGE_SIZE_PARAMS}
            rm -f ${DIR_PREFIX}/${GROUP}/${sched}_parallel.txt
            echo "Num threads: ${NTHREADS}" | tee -a ${DIR_PREFIX}/${GROUP}/${sched}_parallel.txt
            echo "Pin string: ${PIN_STRING}" | tee -a ${DIR_PREFIX}/${GROUP}/${sched}_parallel.txt
            echo "Running profiler tests for parallel ${sched} schedule..."
            for i in $(seq 1 3); do
                likwid-perfctr -C ${PIN_STRING} -g ${GROUP} -m ./blur_aot | tee -a ${DIR_PREFIX}/${GROUP}/${sched}_parallel.txt ;
            done
        fi

        sched_id=$((sched_id + 1))
    done
fi

# Time tests
if [ "${MEASURE_TIME}" -ne "0" ]; then
    # Serial schedules
    sched_id=1
    for sched in ${SCHEDULES}; do
        if [ "${RUN_SERIAL}" -ne "0" ]; then
            export HL_TARGET="${ARCH}-profile"
            export HL_JIT_TARGET="${ARCH}-profile"
            make clean && make SCHEDULE=${sched_id} ${IMAGE_SIZE_PARAMS}
            rm -f ${DIR_PREFIX}/TIME/${sched}_serial.txt
            echo "Running time tests for serial ${sched} schedule..."
            for i in $(seq 1 3); do
                likwid-pin -c 0 ./blur_aot | grep -v likwid-pin | tee -a ${DIR_PREFIX}/TIME/${sched}_serial.txt ;
            done
        fi

        sched_id=$((sched_id + 1))
    done

    # Parallel schedules
    sched_id=1
    for sched in ${SCHEDULES}; do
        if [ "${RUN_PARALLEL}" -ne "0" -a "$sched_id" -ne "3" ]; then
            export HL_TARGET="host-profile"
            export HL_JIT_TARGET="host-profile"
            export HL_NUM_THREADS=${NTHREADS}
            #export OMP_NUM_THREADS="${HL_NUM_THREADS}"
            make clean && make SCHEDULE=${sched_id} PARALLEL=y ${IMAGE_SIZE_PARAMS}
            rm -f ${DIR_PREFIX}/TIME/${sched}_parallel.txt
            echo "Num threads: ${NTHREADS}" | tee -a ${DIR_PREFIX}/TIME/${sched}_parallel.txt
            echo "Pin string: ${PIN_STRING}" | tee -a ${DIR_PREFIX}/TIME/${sched}_parallel.txt
            echo "Running time tests for parallel ${sched} schedule..."
            for i in $(seq 1 3); do
                likwid-pin -c ${PIN_STRING} ./blur_aot | grep -v likwid-pin | tee -a ${DIR_PREFIX}/TIME/${sched}_parallel.txt ;
            done
        fi

        sched_id=$((sched_id + 1))
    done
fi
