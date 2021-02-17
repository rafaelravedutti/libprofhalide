#!/bin/bash

while getopts "a:A:s:g:t:C:r:w:h:c:Sm" flag; do
    case "${flag}" in
        a) algorithm=${OPTARG};;
        A) arch=${OPTARG};;
        s) schedule=${OPTARG};;
        g) group=${OPTARG};;
        t) nthreads=${OPTARG};;
        C) pin_flags=${OPTARG};;
        r) nruns=${OPTARG};;
        w) img_width=${OPTARG};;
        h) img_height=${OPTARG};;
        c) img_channels=${OPTARG};;
        S) save_output=1;;
        m) markers=1;;
    esac
done

# Algorithm
ALGORITHM="${algorithm:-blur}"

# Target architecture
ARCH="${arch:-host}"
#ARCH="${arch:-host-x86-64}" # no vectorization

# Number of threads (default is serial version)
NTHREADS="${nthreads:-1}"

# Pin flags
if [ -n "$pin_flags" ]; then
    PIN_FLAGS="-C ${pin_flags}"
fi

# Number of runs
NRUNS="${nruns:-1}"

# Image sizes 3840x2160 (4K), 10240x4320 (10K), 10112x10112
# Channels are usually 1 or 3
IMAGE_WIDTH="${img_width:-10240}"
IMAGE_HEIGHT="${img_height:-4320}"
IMAGE_CHANNELS="${img_channels:-3}"
IMAGE_SIZE="${IMAGE_WIDTH}x${IMAGE_HEIGHT}x${IMAGE_CHANNELS}"
IMAGE_SIZE_PARAMS="IMAGE_WIDTH=${IMAGE_WIDTH} IMAGE_HEIGHT=${IMAGE_HEIGHT} IMAGE_CHANNELS=${IMAGE_CHANNELS}"

# Treated hostname (specific to RRZE test and woody clusters)
TREATED_HOST=$(hostname | sed -E "s/tg09[0-4]/rome/" | sed "s/medusa/cascadelake/")

# Check for schedule
if [ -z "$schedule" ]; then
    echo "Undefined schedule!"
    exit
fi

# Check for group
if [ -z "$group" ]; then
    echo "Undefined group!"
    exit
fi

# Just to keep uppercase versions
SCHEDULE=$schedule
GROUP=$group

# Save output to file?
SAVE_OUTPUT="${save_output:-0}"

# Use markers or profile globally?
MARKERS="${markers:-0}"

# Define directory structure and filename for results
OUTPUT_PATH="results/${TREATED_HOST}/${ALGORITHM}/${IMAGE_SIZE}/${GROUP%%:*}"
if [ "${NTHREADS}" -eq "1" ]; then
    OUTPUT_FILE="${OUTPUT_PATH}/${SCHEDULE}_serial.txt"
else
    OUTPUT_FILE="${OUTPUT_PATH}/${SCHEDULE}_parallel_${NTHREADS}t.txt"
fi

# Retrieve library paths
source source.me
make clean

number_regex='^[0-9]+$'
if ! [[ $SCHEDULE =~ $number_regex ]] ; then
    case $SCHEDULE in
        breadth_first)  SCHEDULE_ID=1;;    
        full_fusion)    SCHEDULE_ID=2;;    
        sliding_window) SCHEDULE_ID=3;;    
        tile_32x32)     SCHEDULE_ID=4;;    
        *) echo "Invalid schedule: ${SCHEDULE}" && exit ;;    
    esac
else
    SCHEDULE_ID=$SCHEDULE
fi

print_and_save() {
    if [ "${SAVE_OUTPUT}" -ne "0" ]; then
        echo $1 | tee -a ${OUTPUT_FILE}
    else
        echo $1
    fi
}

if [ "${SAVE_OUTPUT}" -ne "0" ]; then
    mkdir -p ${OUTPUT_PATH}
    rm -f ${OUTPUT_FILE}
    likwid-topology -g > "results/${TREATED_HOST}/topology.txt"
    print_and_save "Output submitted to: ${OUTPUT_FILE}"
fi

print_and_save "Algorithm: ${ALGORITHM}, Arch: ${ARCH}, Host: ${TREATED_HOST}, Group: ${GROUP}"
print_and_save "Number of runs: ${NRUNS}, Schedule: ${SCHEDULE}, Image dimensions: ${IMAGE_SIZE}"
print_and_save "Number of threads: ${NTHREADS}, Pin flags: ${PIN_FLAGS}"

if [ "${NTHREADS}" -ne "1" ]; then
    export HL_NUM_THREADS=${NTHREADS}
    EXTRA_FLAGS="PARALLEL=y"
else
    EXTRA_FLAGS=""
fi

if [ "${GROUP}" == "TIME" ]; then
    export HL_TARGET="host-profile"
    export HL_JIT_TARGET="host-profile"
    make SCHEDULE=${SCHEDULE_ID} ${IMAGE_SIZE_PARAMS} ${EXTRA_FLAGS}
    if [ -z "${PIN_FLAGS}" ]; then
        COMMAND="./blur_aot"
    else
        COMMAND="likwid-pin ${PIN_FLAGS} ./blur_aot"
    fi
else
    if [ "${MARKERS}" -eq "1" ]; then
        export HL_TARGET="${ARCH}-perfctr"
        export HL_JIT_TARGET="${ARCH}-perfctr"
        MARKER_FLAG="-m"
    else
        MARKER_FLAG=""
    fi

    make SCHEDULE=${SCHEDULE_ID} PROFILE=y ${IMAGE_SIZE_PARAMS} ${EXTRA_FLAGS}
    COMMAND="likwid-perfctr ${PIN_FLAGS} -g ${GROUP} ${MARKER_FLAG} ./blur_aot"
fi

for i in $(seq 1 ${NRUNS}); do
    if [ "${SAVE_OUTPUT}" -ne "0" ]; then
        ${COMMAND} | tee -a ${OUTPUT_FILE}
    else
        ${COMMAND}
    fi
done
