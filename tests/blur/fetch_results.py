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

def func2region(schedule, func_name):
    if 'breadth_first' in schedule:
        return 'blur_x_prod' if func_name == 'blur_x' else 'blur_x_cons'

    if 'full_fusion' in schedule:
        return 'blur_y_prod'

    #if 'tile_32x32' in schedule:
    #    return 'blur_x_prod' if func_name == 'blur_x' else 'blur_x_cons'

    return 'blur_y.s0.c'

#path_prefix="results/casclakesp2/blur/10240x4320x3"
# Group, Regex, Column, Reduce and Policy functions
#events=[
#    ("TIME",         "TIME",                                     0, sum, min),
#    ("MEM",          "Memory bandwidth",                         2, avg, first),
#    ("CACHES_MOD",   "L1 to L2 evict data volume",               2, sum, first),
#    ("CACHES_MOD",   "L2 to L1 load data volume",                2, sum, first),
#    ("CACHES_MOD",   "L1D_REPLACEMENT",                          3, sum, min),
#    ("FLOPS_SP",     "AVX SP",                                   2, sum, first),
#    ("FLOPS_SP",     "FP_ARITH_INST_RETIRED_256B_PACKED_SINGLE", 3, sum, min)
#]

path_prefix="results/rome/blur/10240x4320x3"
events=[
    ("TIME",         "TIME",                                     0, sum, min),
    ("MEM",          "Memory bandwidth",                         2, avg, first),
    ("CACHE",        "data cache misses",                        2, sum, min),
    ("CACHE",        "data cache miss rate",                     2, avg, first),
    ("CACHE",        "data cache miss ratio",                    2, avg, first),
    ("L2",           "L2D load data volume",                     2, avg, first),
    ("FLOPS_SP",     "SP \[MFLOP\/s\]",                          2, sum, first),
    ("FLOPS_SP",     "RETIRED_SSE_AVX_FLOPS_ALL",                3, sum, min)
]

region_pattern = re.compile("^Region")
pin_pattern = re.compile("^Pin flags: ")
time_pattern = re.compile("blur_x:|blur_y:")
stat_pattern = re.compile("STAT")
results = {}
pinning = {}
event_id = 0

#nthreads = [2, 4, 10, 20]
nthreads = [2, 4, 8, 16]
path_suffixes = ["_serial.txt"] + ["_parallel_" + str(t) + "t.txt" for t in nthreads]

for event in events:
    group = event[0]
    regex = event[1]
    column = event[2]
    policy_fn = event[4]
    pattern = re.compile(regex)
    path = path_prefix + "/" + group
    files = [f for f in listdir(path) if isfile(join(path, f)) and any([f.endswith(suffix) for suffix in path_suffixes])]

    for f in files:
        filename = path + "/" + f
        is_serial = "parallel" not in filename
        schedule, _ = f.split('.')
        if schedule not in results:
            results[schedule] = {}

        schedule_results = results[schedule]
        with open(filename, 'r') as fp:
            region = None
            for line in fp.readlines():
                if event_id == 0:
                    if time_pattern.search(line):
                        splitted_text = re.sub(' +', ' ', line.strip()).split(' ')
                        func = splitted_text[0][:-1]
                        value = num(splitted_text[1][:-2])
                        region = func2region(schedule, func)

                        if region not in schedule_results:
                            schedule_results[region] = {}

                        region_results = schedule_results[region]
                        if event_id not in region_results:
                            region_results[event_id] = { func: value }
                        else:
                            if func not in region_results[event_id]:
                                region_results[event_id][func] = value
                            else:
                                region_results[event_id][func] = policy_fn([region_results[event_id][func], value])

                else:
                    if region_pattern.search(line):
                        region, _ = line[7:].split(',')

                    if pin_pattern.search(line) and schedule not in pinning:
                        pinning[schedule] = line[11:].strip()

                    if pattern.search(line) and (is_serial or stat_pattern.search(line)):
                        value = num(line.split('|')[column].strip())

                        if region not in schedule_results:
                            schedule_results[region] = {}

                        region_results = schedule_results[region]
                        if event_id not in region_results:
                            region_results[event_id] = value
                        else:
                            region_results[event_id] = policy_fn([region_results[event_id], value])

    event_id += 1

sorted_schedules = []
for suffix in path_suffixes:
    for schedule in results:
        if schedule.endswith(suffix[:-4]):
            sorted_schedules.append(schedule)

header = "Cascade Lake,Pin String,Region"
for event in events:
    header += ',' + event[1]

print(header)
for schedule in sorted_schedules:
    schedule_results = results[schedule]
    pin_string = pinning[schedule] if schedule in pinning else ""
    reduced_events = {}
    for region in schedule_results:
        region_results = schedule_results[region]
        output = schedule + ",\"" + pin_string + "\"," + region

        for event_id in region_results:
            value = None

            if event_id == 0:
                reduce_fn = events[event_id][3]
                value = reduce_fn([region_results[event_id][f] for f in region_results[event_id]])
            else:
                value = region_results[event_id]

            if event_id not in reduced_events:
                reduced_events[event_id] = [value]
            else:
                reduced_events[event_id].append(value)

            output += "," + str(value)

        print(output)

    if len(schedule_results) > 1:
        output = schedule + ",\"" + pin_string + "\",all"
        for event_id in reduced_events:
            reduce_fn = events[event_id][3]
            value = reduce_fn(reduced_events[event_id])
            output += "," + str(value)


        print(output)

