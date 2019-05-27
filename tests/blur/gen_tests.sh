#!/bin/bash

echo "Running profiler tests for breadth-first schedule..."
make clean && make SCHEDULE=1 PROFILE=y
./blur_aot | tee blur_breadth_first_profile_$(hostname).csv

echo "Running time tests for breadth-first schedule..."
make clean && make SCHEDULE=1
./blur_aot | tee blur_breadth_first_time_$(hostname).csv

echo "Running profiler tests for full-fusion schedule..."
make clean && make SCHEDULE=2 PROFILE=y
./blur_aot | tee blur_full_fusion_profile_$(hostname).csv

echo "Running time tests for full-fusion schedule..."
make clean && make SCHEDULE=2
./blur_aot | tee blur_full_fusion_time_$(hostname).csv

echo "Running profiler tests for sliding window schedule..."
make clean && make SCHEDULE=3 PROFILE=y
./blur_aot | tee blur_sliding_window_profile_$(hostname).csv

echo "Running time tests for sliding window schedule..."
make clean && make SCHEDULE=3
./blur_aot | tee blur_sliding_window_time_$(hostname).csv

echo "Running profiler tests for tile 32x32 schedule..."
make clean && make SCHEDULE=4 PROFILE=y
./blur_aot | tee blur_tile_32x32_profile_$(hostname).csv

echo "Running time tests for tile 32x32 schedule..."
make clean && make SCHEDULE=4
./blur_aot | tee blur_tile_32x32_time_$(hostname).csv
