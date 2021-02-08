from os import listdir
from os.path import isfile, join
import re

def avg(lst):
    return sum(lst) / len(lst)

def first(lst):
    return lst[0]

def num(s):
    try:
        return int(s)
    except ValueError:
        return float(s)

# Group, Regex, Column, Reduce and Policy functions
events=[
    ("MEM",      "Memory bandwidth",                         2, avg, first),
    ("CACHES",   "L1 to L2 evict data volume",               2, sum, first),
    ("CACHES",   "L2 to L1 load data volume",                2, sum, first),
    ("CACHES",   "L1D_REPLACEMENT",                          3, sum, min),
    ("FLOPS_SP", "AVX SP",                                   2, sum, first),
    ("FLOPS_SP", "FP_ARITH_INST_RETIRED_256B_PACKED_SINGLE", 3, sum, min)
]

path_prefix="results/cascadelake/blur/10240x4320x3"
path_suffix="_serial.txt"
region_pattern = re.compile("^Region")
results = {}
event_id = 0

for event in events:
    group = event[0]
    regex = event[1]
    column = event[2]
    policy_fn = event[4]
    pattern = re.compile(regex)
    path = path_prefix + "/" + group
    files = [f for f in listdir(path) if isfile(join(path, f)) and f.endswith(path_suffix)]

    for f in files:
        filename = path + "/" + f
        schedule, _ = f.split('.')
        if schedule not in results:
            results[schedule] = {}

        schedule_results = results[schedule]
        with open(filename, 'r') as fp:
            region = None
            for line in fp.readlines():
                if region_pattern.search(line):
                    region, _ = line[7:].split(',')

                if pattern.search(line):
                    value = num(line.split('|')[column].strip())

                    if region not in schedule_results:
                        schedule_results[region] = {}

                    region_results = schedule_results[region]
                    if event_id not in region_results:
                        region_results[event_id] = value
                    else:
                        region_results[event_id] = policy_fn([region_results[event_id], value])

    event_id += 1

for schedule in results:
    schedule_results = results[schedule]
    reduced_events = {}
    for region in schedule_results:
        region_results = schedule_results[region]
        output = schedule + "," + region

        for event_id in region_results:
            value = region_results[event_id]
            if event_id not in reduced_events:
                reduced_events[event_id] = [value]
            else:
                reduced_events[event_id].append(value)

            output += "," + str(value)

        print(output)

    if len(schedule_results) > 1:
        output = schedule + ",all"
        for event_id in reduced_events:
            reduce_fn = events[event_id][3]
            value = reduce_fn(reduced_events[event_id])
            output += "," + str(value)


        print(output)

