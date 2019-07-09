#!/bin/bash

# Image size
IMG_SIZE="10K"

# Schedules
SCHEDULES="serial serial_vec parallel_with_vec"

# Extra flags
EXTRA_FLAGS=""
#EXTRA_FLAGS="VECTORIZE=y"

sched_id=1
for sched in ${SCHEDULES}; do
  if [ "$sched_id" -ne "3" ]; then
    # Profiler tests
    echo "Running profiler tests for serial ${sched} schedule..."
    make clean && make SCHEDULE=${sched_id} PROFILE=y ${EXTRA_FLAGS}
    for i in $(seq 1 30); do ./local_laplacian_aot | tee -a csv/local_laplacian_${sched}_profile_$(hostname)_${IMG_SIZE}.csv ; done

    # Time tests
    echo "Running time tests for serial ${sched} schedule..."
    make clean && make SCHEDULE=${sched_id} ${EXTRA_FLAGS}
    for i in $(seq 1 30); do ./local_laplacian_aot | tee -a csv/local_laplacian_${sched}_time_$(hostname)_${IMG_SIZE}.csv ; done
  else
    # Profiler tests
    echo "Running profiler tests for parallel ${sched} schedule..."
    make clean && make SCHEDULE=${sched_id} PARALLEL=y PROFILE=y ${EXTRA_FLAGS}
    for i in $(seq 1 30); do /home/soft/likwid/bin/likwid-pin -c 3 ./local_laplacian_aot | grep -v likwid-pin | tee -a csv/local_laplacian_${sched}_profile_$(hostname)_${IMG_SIZE}.csv ; done

    # Time tests
    echo "Running time tests for parallel ${sched} schedule..."
    make clean && make SCHEDULE=${sched_id} PARALLEL=y ${EXTRA_FLAGS}
    for i in $(seq 1 30); do /home/soft/likwid/bin/likwid-pin -c 3 ./local_laplacian_aot | grep -v likwid-pin | tee -a csv/local_laplacian_${sched}_time_$(hostname)_${IMG_SIZE}.csv ; done
  fi

  sched_id=$((sched_id + 1))
done
