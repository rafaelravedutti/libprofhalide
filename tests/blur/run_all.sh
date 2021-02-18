#!/bin/bash

# Schedules
SCHEDULES="breadth_first full_fusion sliding_window tile_32x32"

# Number of threads and pinning string on parallel schedules
NTHREADS=(1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20)
PIN_FLAGS=("0" "M0:2,0-1" "M0:3,0-2" "M0:4,0-3" "M0:5,0-4" "M0:6,0-5" "M0:7,0-6" "M0:8,0-7" "M0:9,0-8" "M0:10,0-9" "M0:11,0-10" "M0:12,0-11" "M0:13,0-12" "M0:14,0-13" "M0:15,0-14" "M0:16,0-15" "M0:17,0-16" "M0:18,0-17" "M0:19,0-18" "M0:20,0-19")

# Performance groups (Likwid)
PERF_GROUPS="TIME MEM"
#PERF_GROUPS="TIME FLOPS_SP CACHES_MOD MEM"
#PERF_GROUPS="TIME FLOPS_SP CACHE L2 MEM"

# Profiler tests
for perf_group in ${PERF_GROUPS}; do
    echo "Running for perf_group=${perf_group}..."
    for thread_config_id in "${!NTHREADS[@]}"; do
        nthreads=${NTHREADS[$thread_config_id]}
        pin_flags=${PIN_FLAGS[$thread_config_id]}
        echo "Running for nthreads=${nthreads}, pin_flags=\"${pin_flags}\"..."
        for sched in ${SCHEDULES}; do
            if [ "${nthreads}" -eq "1" -o "$sched" != "sliding_window" ]; then
                echo "Running for sched=${sched}..."
                #bash run_test.sh -s "${sched}" -g "${perf_group}" -t "${nthreads}" -C "${pin_flags}" -r 3 -S -m
                bash run_test.sh -s "${sched}" -g "${perf_group}" -t "${nthreads}" -r 3 -S -n
            fi
        done
    done
done
