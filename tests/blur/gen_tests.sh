#!/bin/bash

# Image size
IMG_SIZE="10K"

# Serial schedules
#echo "Running profiler tests for serial breadth-first schedule..."
#make clean && make SCHEDULE=1 PROFILE=y
#for i in $(seq 1 30); do ./blur_aot | tee -a csv/blur_breadth_first_serial_profile_$(hostname)_${IMG_SIZE}.csv ; done

#echo "Running time tests for serial breadth-first schedule..."
#make clean && make SCHEDULE=1
#./blur_aot | tee csv/blur_breadth_first_serial_time_$(hostname)_${IMG_SIZE}.csv

#echo "Running profiler tests for serial full-fusion schedule..."
#make clean && make SCHEDULE=2 PROFILE=y
#for i in $(seq 1 30); do ./blur_aot | tee -a csv/blur_full_fusion_serial_profile_$(hostname)_${IMG_SIZE}.csv ; done

#echo "Running time tests for serial full-fusion schedule..."
#make clean && make SCHEDULE=2
#./blur_aot | tee csv/blur_full_fusion_serial_time_$(hostname).csv

#echo "Running profiler tests for serial sliding window schedule..."
#make clean && make SCHEDULE=3 PROFILE=y
#for i in $(seq 1 30); do ./blur_aot | tee -a csv/blur_sliding_window_serial_profile_$(hostname)_${IMG_SIZE}.csv ; done

#echo "Running time tests for serial sliding window schedule..."
#make clean && make SCHEDULE=3
#./blur_aot | tee csv/blur_sliding_window_serial_time_$(hostname)_${IMG_SIZE}.csv

#echo "Running profiler tests for serial tile 32x32 schedule..."
#make clean && make SCHEDULE=4 PROFILE=y
#for i in $(seq 1 30); do ./blur_aot | tee -a csv/blur_tile_32x32_serial_profile_$(hostname)_${IMG_SIZE}.csv ; done

#echo "Running time tests for serial tile 32x32 schedule..."
#make clean && make SCHEDULE=4
#./blur_aot | tee csv/blur_tile_32x32_serial_time_$(hostname)_${IMG_SIZE}.csv

# Parallel schedules
echo "Running profiler tests for parallel breadth-first schedule..."
make clean && make SCHEDULE=1 PARALLEL=y PROFILE=y
for i in $(seq 1 30); do /home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | grep -v likwid-pin | tee -a csv/blur_breadth_first_parallel_profile_$(hostname)_${IMG_SIZE}.csv ; done

#echo "Running time tests for breadth-first schedule..."
#make clean && make SCHEDULE=1 PARALLEL=y
#/home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | tee csv/blur_breadth_first_parallel_time_$(hostname)_${IMG_SIZE}.csv

#echo "Running profiler tests for full-fusion schedule..."
make clean && make SCHEDULE=2 PARALLEL=y PROFILE=y
for i in $(seq 1 30); do /home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | grep -v likwid-pin | tee -a csv/blur_full_fusion_parallel_profile_$(hostname)_${IMG_SIZE}.csv ; done

#echo "Running time tests for full-fusion schedule..."
#make clean && make SCHEDULE=2 PARALLEL=y
#/home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | tee csv/blur_full_fusion_parallel_time_$(hostname)_${IMG_SIZE}.csv

#echo "Running profiler tests for sliding window schedule..."
#make clean && make SCHEDULE=3 PARALLEL=y PROFILE=y
#for i in $(seq 1 30); do /home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | tee csv/blur_sliding_window_parallel_profile_$(hostname)_${IMG_SIZE}.csv ; done

#echo "Running time tests for sliding window schedule..."
#make clean && make SCHEDULE=3 PARALLEL=y
#/home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | tee csv/blur_sliding_window_parallel_time_$(hostname)_${IMG_SIZE}.csv

#echo "Running profiler tests for tile 32x32 schedule..."
#make clean && make SCHEDULE=4 PARALLEL=y PROFILE=y
#for i in $(seq 1 30); do /home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | grep -v likwid-pin | tee -a csv/blur_tile_32x32_parallel_profile_$(hostname)_${IMG_SIZE}.csv ; done

#echo "Running time tests for tile 32x32 schedule..."
#make clean && make SCHEDULE=4 PARALLEL=y
#/home/soft/likwid/bin/likwid-pin -c 3 ./blur_aot | tee csv/blur_tile_32x32_parallel_time_$(hostname)_${IMG_SIZE}.csv
