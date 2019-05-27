#!/bin/bash

echo "Running tests for breadth-first schedule..."
make clean && make SCHEDULE=1
./blur_aot | tee blur_breadth_first.csv

echo "Running tests for full-fusion schedule..."
make clean && make SCHEDULE=2
./blur_aot | tee blur_full_fusion.csv

echo "Running tests for sliding window schedule..."
make clean && make SCHEDULE=3
./blur_aot | tee blur_sliding_window.csv

echo "Running tests for tile 32x32 schedule..."
make clean && make SCHEDULE=4
./blur_aot | tee blur_tile_32x32.csv
