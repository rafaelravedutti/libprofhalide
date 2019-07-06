#!/bin/bash

# Image size
IMG_SIZE="vec_4K"

# Schedules
SCHEDULES="breadth_first full_fusion sliding_window tile_32x32"

# Extra flags
EXTRA_FLAGS="VECTORIZE=y"

sched_id=1
for sched in ${SCHEDULES}; do
  # Profiler tests
  echo "Running profiler tests for serial ${sched} schedule..."
  make clean && make SCHEDULE=${sched_id} PROFILE=y ${EXTRA_FLAGS}
  for i in $(seq 1 30); do ./blur_aot | tee -a csv/blur_${sched}_serial_profile_$(hostname)_${IMG_SIZE}.csv ; done

  # Time tests
  echo "Running time tests for serial ${sched} schedule..."
  make clean && make SCHEDULE=${sched_id} ${EXTRA_FLAGS}
  for i in $(seq 1 30); do ./blur_aot | tee -a csv/blur_${sched}_serial_time_$(hostname)_${IMG_SIZE}.csv ; done

  sched_id=$((sched_id + 1))
done

sched_id=1
for sched in ${SCHEDULES}; do
  if [ "$sched_id" -ne "3" ]; then
    # Profiler tests
    echo "Running profiler tests for parallel ${sched} schedule..."
    make clean && make SCHEDULE=${sched_id} PARALLEL=y PROFILE=y ${EXTRA_FLAGS}
    for i in $(seq 1 30); do /home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | grep -v likwid-pin | tee -a csv/blur_${sched}_parallel_profile_$(hostname)_${IMG_SIZE}.csv ; done

    # Time tests
    echo "Running time tests for parallel ${sched} schedule..."
    make clean && make SCHEDULE=${sched_id} PARALLEL=y ${EXTRA_FLAGS}
    for i in $(seq 1 30); do /home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | grep -v likwid-pin | tee -a csv/blur_${sched}_parallel_time_$(hostname)_${IMG_SIZE}.csv ; done
  fi

  sched_id=$((sched_id + 1))
done
