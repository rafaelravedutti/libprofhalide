#!/bin/bash

# Schedules
SCHEDULES="breadth_first full_fusion sliding_window tile_32x32"

# Target architecture
ARCH="host"
#ARCH="host-x86-64" # no vectorization

# Group (Likwid)
GROUP=MEM_SP

# Extra flags
EXTRA_FLAGS=""
#EXTRA_FLAGS="VECTORIZE=y"

# Image size
FILENAME_SUFFIX="${GROUP}_10K"

# Run serial and/or parallel schedules
RUN_SERIAL=1
RUN_PARALLEL=0

# Profiler tests
sched_id=1
for sched in ${SCHEDULES}; do
  if [ "${RUN_SERIAL}" -ne "0" ]; then
      export HL_TARGET="${ARCH}-perfctr"
      export HL_JIT_TARGET="${ARCH}-perfctr"
      make clean && make SCHEDULE=${sched_id} PROFILE=y ${EXTRA_FLAGS}
      echo "Running profiler tests for serial ${sched} schedule..."
      for i in $(seq 1 3); do likwid-perfctr -C 3 -g ${GROUP} -m ./blur_aot | tee -a csv/blur_${sched}_serial_profile_$(hostname)_${FILENAME_SUFFIX}.csv ; done
      #for i in $(seq 1 3); do ./blur_aot | tee -a csv/blur_${sched}_serial_profile_$(hostname)_${FILENAME_SUFFIX}.csv ; done
  fi

  sched_id=$((sched_id + 1))
done

# Time tests
sched_id=1
for sched in ${SCHEDULES}; do
  if [ "${RUN_SERIAL}" -ne "0" ]; then
      export HL_TARGET="${ARCH}-profile"
      export HL_JIT_TARGET="${ARCH}-profile"
      make clean && make SCHEDULE=${sched_id} ${EXTRA_FLAGS}
      echo "Running time tests for serial ${sched} schedule..."
      for i in $(seq 1 3); do likwid-pin -c 3 ./blur_aot | grep -v likwid-pin | tee -a csv/blur_${sched}_serial_time_$(hostname)_${FILENAME_SUFFIX}.csv ; done
  fi

  sched_id=$((sched_id + 1))
done

# Profiler tests
sched_id=1
for sched in ${SCHEDULES}; do
  if [ "${RUN_PARALLEL}" -ne "0" -a "$sched_id" -ne "3" ]; then
    export HL_TARGET="${ARCH}-perfctr"
    export HL_JIT_TARGET="${ARCH}-perfctr"
    make clean && make SCHEDULE=${sched_id} PARALLEL=y PROFILE=y ${EXTRA_FLAGS}
    echo "Running profiler tests for parallel ${sched} schedule..."
    #for i in $(seq 1 3); do likwid-perfctr -g ${GROUP} -m ./blur_aot | tee -a csv/blur_${sched}_parallel_profile_$(hostname)_${FILENAME_SUFFIX}.csv ; done
    for i in $(seq 1 3); do likwid-perfctr -C 0-3 -g ${GROUP} -m ./blur_aot | tee -a csv/blur_${sched}_parallel_profile_$(hostname)_${FILENAME_SUFFIX}.csv ; done
    #for i in $(seq 1 3); do likwid-pin -C 0-3 ./blur_aot | grep -v likwid-pin | tee -a csv/blur_${sched}_parallel_profile_$(hostname)_${FILENAME_SUFFIX}.csv ; done
  fi

  sched_id=$((sched_id + 1))
done

# Time tests
sched_id=1
for sched in ${SCHEDULES}; do
  if [ "${RUN_PARALLEL}" -ne "0" -a "$sched_id" -ne "3" ]; then
    export HL_TARGET="host-profile"
    export HL_JIT_TARGET="host-profile"
    make clean && make SCHEDULE=${sched_id} PARALLEL=y ${EXTRA_FLAGS}
    echo "Running time tests for parallel ${sched} schedule..."
    for i in $(seq 1 3); do likwid-pin -c 0-3 ./blur_aot | grep -v likwid-pin | tee -a csv/blur_${sched}_parallel_time_$(hostname)_${FILENAME_SUFFIX}.csv ; done
  fi

  sched_id=$((sched_id + 1))
done
