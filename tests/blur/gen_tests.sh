#!/bin/bash

# Algorithm
ALGORITHM="blur"

# Schedules
SCHEDULES="breadth_first full_fusion sliding_window tile_32x32"

# Target architecture
ARCH="host"
#ARCH="host-x86-64" # no vectorization

# Number of threads and pinning string on parallel schedules
NTHREADS=(2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20)
PIN_FLAGS=("-C M0:2,0-1" "-C M0:3,0-2" "-C M0:4,0-3" "-C M0:5,0-4" "-C M0:6,0-5" "-C M0:7,0-6" "-C M0:8,0-7" "-C M0:9,0-8" "-C M0:10,0-9" "-C M0:11,0-10" "-C M0:12,0-11" "-C M0:13,0-12" "-C M0:14,0-13" "-C M0:15,0-14" "-C M0:16,0-15" "-C M0:17,0-16" "-C M0:18,0-17" "-C M0:19,0-18" "-C M0:20,0-19")

# Performance groups (Likwid)
PERF_GROUPS="FLOPS_SP CACHES_MOD MEM"
#PERF_GROUPS="FLOPS_SP CACHE L2 MEM"

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
RUN_SERIAL=1
RUN_PARALLEL=1

# Measure events and/or time
MEASURE_EVENTS=1
MEASURE_TIME=1

# Treated hostname (specific to RRZE test and woody clusters)
TREATED_HOST=$(hostname | sed -E "s/tg09[0-4]/rome/" | sed "s/medusa/cascadelake/")

# Retrieve library paths
source source.me

# Define directory structure for results
DIR_PREFIX="results/${TREATED_HOST}/${ALGORITHM}/${IMAGE_SIZE}"

# Print topology
likwid-topology -g > "results/${TREATED_HOST}/topology.txt"

# Profiler tests
if [ "${MEASURE_EVENTS}" -ne "0" ]; then
    for perf_group in ${PERF_GROUPS}; do
        echo "Running for performance group ${perf_group}..."
        perf_group_dir=${perf_group%%:*}
        mkdir -p ${DIR_PREFIX}/${perf_group_dir}

        # Serial schedules
        sched_id=1
        for sched in ${SCHEDULES}; do
            if [ "${RUN_SERIAL}" -ne "0" ]; then
                FILENAME=${DIR_PREFIX}/${perf_group_dir}/${sched}_serial.txt
                export HL_TARGET="${ARCH}-perfctr"
                export HL_JIT_TARGET="${ARCH}-perfctr"
                make clean && make SCHEDULE=${sched_id} PROFILE=y ${IMAGE_SIZE_PARAMS}
                rm -f ${FILENAME}
                echo "Running profiler tests for serial ${sched} schedule..."
                for i in $(seq 1 3); do
                    likwid-perfctr -C 0 -g ${perf_group} -m ./blur_aot | tee -a ${FILENAME} ;
                done
            fi

            sched_id=$((sched_id + 1))
        done


        # Parallel schedules
        for thread_config_id in "${!NTHREADS[@]}"; do
            nthreads=${NTHREADS[$thread_config_id]}
            pin_flags=${PIN_FLAGS[$thread_config_id]}
            sched_id=1
            for sched in ${SCHEDULES}; do
                if [ "${RUN_PARALLEL}" -ne "0" -a "$sched_id" -ne "3" ]; then
                    FILENAME=${DIR_PREFIX}/${perf_group_dir}/${sched}_parallel_${nthreads}t.txt
                    export HL_TARGET="${ARCH}-perfctr"
                    export HL_JIT_TARGET="${ARCH}-perfctr"
                    export HL_NUM_THREADS=${nthreads}
                    #export OMP_NUM_THREADS="${HL_NUM_THREADS}"
                    make clean && make SCHEDULE=${sched_id} PARALLEL=y PROFILE=y ${IMAGE_SIZE_PARAMS}
                    rm -f ${FILENAME}
                    echo "Num threads: ${nthreads}" | tee -a ${FILENAME}
                    echo "Pin flags: ${pin_flags}" | tee -a ${FILENAME}
                    echo "Running profiler tests for parallel ${sched} schedule..."
                    for i in $(seq 1 3); do
                        likwid-perfctr ${pin_flags} -g ${perf_group} -m ./blur_aot | tee -a ${FILENAME} ;
                    done
                fi

                sched_id=$((sched_id + 1))
            done
        done
    done
fi

# Time tests
if [ "${MEASURE_TIME}" -ne "0" ]; then
    mkdir -p ${DIR_PREFIX}/TIME

    # Serial schedules
    sched_id=1
    for sched in ${SCHEDULES}; do
        if [ "${RUN_SERIAL}" -ne "0" ]; then
            FILENAME=${DIR_PREFIX}/TIME/${sched}_serial.txt
            export HL_TARGET="${ARCH}-profile"
            export HL_JIT_TARGET="${ARCH}-profile"
            make clean && make SCHEDULE=${sched_id} ${IMAGE_SIZE_PARAMS}
            rm -f ${FILENAME}
            echo "Running time tests for serial ${sched} schedule..."
            for i in $(seq 1 3); do
                likwid-pin -c 0 ./blur_aot | grep -v likwid-pin | tee -a ${FILENAME} ;
            done
        fi

        sched_id=$((sched_id + 1))
    done

    # Parallel schedules
    for thread_config_id in "${!NTHREADS[@]}"; do
        nthreads=${NTHREADS[$thread_config_id]}
        pin_flags=${PIN_FLAGS[$thread_config_id]}
        sched_id=1
        for sched in ${SCHEDULES}; do
            if [ "${RUN_PARALLEL}" -ne "0" -a "$sched_id" -ne "3" ]; then
                FILENAME=${DIR_PREFIX}/TIME/${sched}_parallel_${nthreads}t.txt
                export HL_TARGET="host-profile"
                export HL_JIT_TARGET="host-profile"
                export HL_NUM_THREADS=${nthreads}
                #export OMP_NUM_THREADS="${HL_NUM_THREADS}"
                make clean && make SCHEDULE=${sched_id} PARALLEL=y ${IMAGE_SIZE_PARAMS}
                rm -f ${FILENAME}
                echo "Num threads: ${nthreads}" | tee -a ${FILENAME}
                echo "Pin string: ${pin_flags}" | tee -a ${FILENAME}
                echo "Running time tests for parallel ${sched} schedule..."
                for i in $(seq 1 3); do
                    likwid-pin ${pin_flags} ./blur_aot | grep -v likwid-pin | tee -a ${FILENAME} ;
                done
            fi

            sched_id=$((sched_id + 1))
        done
    done
fi
