import csv
import matplotlib.pyplot as plt
import numpy as np

def get_file_results(filename, profile_file, image_size, schedule):
  time_data = {}
  time_iter = {}

  if profile_file:
    profile_data = {}
    profile_iter = {}
  else:
    profile_data = None
    profile_iter = None

  try:
    with open(filename) as csvfile:
      csv_data = csv.reader(csvfile, delimiter=',')
      time_results = True

      for row in csv_data:
        columns_number = len(row)

        if columns_number > 1:
          stage = row[0]
          column = 0

          if time_results and stage not in time_data:
            time_data[stage] = {}
            time_iter[stage] = {}

          if not time_results and stage not in profile_data:
            profile_data[stage] = {}
            profile_iter[stage] = {}

          for data in row[1:columns_number]:
            data = float(data)

            if time_results:
              if column not in time_data[stage]:
                time_data[stage][column] = data
                time_iter[stage][column] = 1
              else:
                time_data[stage][column] += data
                time_iter[stage][column] += 1
            else:
              if column not in profile_data[stage]:
                profile_data[stage][column] = data
                profile_iter[stage][column] = 1
              else:
                profile_data[stage][column] += data
                profile_iter[stage][column] += 1

            column += 1

        elif profile_file:
          time_results = not time_results

  except FileNotFoundError:
    time_data = None
    time_iter = None
    profile_data = None
    profile_iter = None

  return {
    'image_size': image_size,
    'schedule': schedule,
    'time_data': time_data,
    'time_iter': time_iter,
    'profile_data': profile_data,
    'profile_iter': profile_iter
  }

def print_results(results):
  if results['time_data'] is None and results['profile_data'] is None:
    print("None")
    return

  time_data = results['time_data']
  time_iter = results['time_iter']
  profile_data = results['profile_data']
  profile_iter = results['profile_iter']

  for stage in time_data:
    time_output = stage

    for data in time_data[stage]:
      time_output += ",{}".format(time_data[stage][data] / time_iter[stage][data])

    print(time_output)

  if profile_data is not None:
    for stage in profile_data:
      profile_output = stage

      for data in profile_data[stage]:
        profile_output += ",{}".format(profile_data[stage][data] / profile_iter[stage][data])

      print(profile_output)

def get_stage_major_name(stage, stage_list):
  for stg in stage_list:
    if stage.startswith(stg):
      return stg

  return None

def is_thread_stage(stage):
  thread_suffixes = ['t0', 't1', 't2', 't3']

  for suffix in thread_suffixes:
    if stage[-2:] == suffix:
      return True

  return False

def get_thread_bar_values(values):
  threads = ['t0', 't1', 't2', 't3']
  bar_values = []

  for thread in threads:
    thread_values = []

    for sched in range(scheds):
      if thread not in values[sched]:
        thread_values.append(0)
      else:
        thread_values.append(values[sched][thread])

    bar_values.append(thread_values)

  return bar_values

def vector_add(v1, v2):
  if len(v1) == len(v2):
    result = []

    for i in range(len(v1)):
      result.append(v1[i] + v2[i])

    return result

  return []

algorithms = ['interpolate']
hostnames = ['i38']
image_sizes = ['4K', '10K']
schedules = ['flat', 'flat_vec', 'flat_par_vec', 'flat_vec_sometimes']
stages = [
  'downsampled[0]', 'downsampled[1]', 'downsampled[2]', 'downsampled[3]', 'downsampled[4]',
  'interpolated[0]', 'interpolated[1]', 'interpolated[2]', 'interpolated[3]', 'interpolated[4]',
  'normalize'
]

colors = [
  '#770000ff', '#990000ff', '#bb0000ff', '#dd0000ff', '#ff0000ff',
  '#007700ff', '#009900ff', '#00bb00ff', '#00dd00ff', '#00ff00ff',
  '#ffff00ff'
]

profile_results = {}
time_results = {}
counter = 0

for algorithm in algorithms:
  for hostname in hostnames:
    for image_size in image_sizes:
      for schedule in schedules:
        filename = "csv/{}_{}_profile_{}_{}.csv".format(algorithm, schedule, hostname, image_size)
        profile_results[counter] = get_file_results(filename, True, image_size, schedule)
        print(filename)
        print_results(profile_results[counter])
        print(" ")

        filename = "csv/{}_{}_time_{}_{}.csv".format(algorithm, schedule, hostname, image_size)
        time_results[counter] = get_file_results(filename, False, image_size, schedule)
        print(filename)
        print_results(time_results[counter])
        print(" ")

        counter += 1

scheds = len(schedules)

plt.rcParams.update({'font.size': 10})
plt.rcParams.update({'hatch.color': 'black'})
sched_labels = [sched.replace("_", "\n") for sched in schedules]
hatches = (' ', '////', '++++', '----')

stage_time_array = {}
stage_cache_miss_array = {}
stage_mflop_array = {}
stage_data_volume_array = {}

for i in range(0, counter, scheds):
  y_pos = np.arange(scheds)
  bar_width = 0.35
  bar_sep = 0.1 

  img_size = None

  stage_time_avg = {}
  stage_cache_miss_avg = {}
  stage_flop_avg = {}
  stage_lines_out_avg = {}
  stage_rqsts_miss_avg = {}

  for stage in stages:
    stage_time_array[stage] = []
    stage_cache_miss_array[stage] = []
    stage_mflop_array[stage] = []
    stage_data_volume_array[stage] = []

  for j in range(scheds):
    idx = i + j

    if img_size is None:
      img_size = time_results[idx]['image_size']

    for stage in stages:
      stage_time_avg[stage] = 0
      stage_cache_miss_avg[stage] = {}
      stage_flop_avg[stage] = {}
      stage_lines_out_avg[stage] = {}
      stage_rqsts_miss_avg[stage] = {}

    if time_results[idx]['time_data'] is not None and profile_results[idx]['profile_data'] is not None:
      tdata = time_results[idx]['time_data']
      titer = time_results[idx]['time_iter']
      pdata = profile_results[idx]['profile_data']
      piter = profile_results[idx]['profile_iter']

      for stg in tdata:
        if stg != 'all' and stg != 'total':
          stage_time_avg[stg] += tdata[stg][0] / titer[stg][0]

      for stg in pdata:
        if is_thread_stage(stg):
          stage = get_stage_major_name(stg, stages)
          thread = stg[-2:]

          if stage is not None and stage in stage_cache_miss_avg:
            if thread not in stage_cache_miss_avg[stage]:
              stage_cache_miss_avg[stage][thread] = 0
              stage_flop_avg[stage][thread] = 0
              stage_lines_out_avg[stage][thread] = 0
              stage_rqsts_miss_avg[stage][thread] = 0

            stage_cache_miss_avg[stage][thread] += pdata[stg][0] / piter[stg][0]
            stage_flop_avg[stage][thread] += pdata[stg][1] / piter[stg][1]
            stage_lines_out_avg[stage][thread] += pdata[stg][2] / piter[stg][2]
            stage_rqsts_miss_avg[stage][thread] += pdata[stg][3] / piter[stg][3]

    stage_mcache_miss_avg = {}
    stage_mflop_avg = {}
    stage_data_volume_avg = {}

    for stage in stages:
      stage_mcache_miss_avg[stage] = {}
      stage_mflop_avg[stage] = {}
      stage_data_volume_avg[stage] = {}

      for t in stage_cache_miss_avg[stage]:
        stage_mcache_miss_avg[stage][t] = 1.E-6 * stage_cache_miss_avg[stage][t]

      for t in stage_flop_avg[stage]:
        stage_mflop_avg[stage][t] = 1.E-6 * stage_flop_avg[stage][t]

      for t in stage_lines_out_avg[stage]:
        stage_data_volume_avg[stage][t] = 1.E-6 * (stage_lines_out_avg[stage][t] + stage_rqsts_miss_avg[stage][t]) * 64

      stage_time_array[stage].append(stage_time_avg[stage])
      stage_cache_miss_array[stage].append(stage_mcache_miss_avg[stage])
      stage_mflop_array[stage].append(stage_mflop_avg[stage])
      stage_data_volume_array[stage].append(stage_data_volume_avg[stage])

  fig = plt.figure()

  color_id = 0
  previous_values = [0, 0, 0, 0]

  for stage in stages:
    plt.bar(
      y_pos, stage_time_array[stage], bar_width - bar_sep * 0.5, color=colors[color_id],
      align='center', alpha=0.5, label=stage, bottom=previous_values)

    previous_values = vector_add(previous_values, stage_time_array[stage])
    color_id += 1

  plt.xticks(y_pos, sched_labels)
  plt.ylabel("Time (ms)")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()
  plt.tight_layout()

  fig.savefig("pdf/time_per_schedule_{}.pdf".format(img_size), bbox_inches = 'tight', pad_inches = 0)

  fig = plt.figure()

  color_id = 0
  previous_values = [0, 0, 0, 0]

  for stage in stages:
    bar_values = get_thread_bar_values(stage_cache_miss_array[stage])
    hatch_idx = 0
    label = stage

    for values in bar_values:
      plt.bar(
        y_pos, values, bar_width, color=colors[color_id], align='center',
        alpha=0.5, label=label, hatch=hatches[hatch_idx], bottom=previous_values)

      label = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

    color_id += 1

  plt.xticks(y_pos, sched_labels)
  plt.ylabel("L1 cache misses")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)

  #if img_size == '4K':
  #  plt.yticks([0, 1, 2, 3, 4], ["0M", "1M", "2M", "3M", "4M"])
  #  plt.ylim(top=5)

  #else:
  #  plt.yticks([0, 5, 10, 15, 20], ["0M", "5M", "10M", "15M", "20M"])
  #  plt.ylim(top=20)

  plt.legend()
  plt.tight_layout()

  fig.savefig("pdf/cache_miss_per_schedule_{}.pdf".format(img_size), bbox_inches = 'tight', pad_inches = 0)

  fig = plt.figure()

  color_id = 0
  previous_values = [0, 0, 0, 0]

  for stage in stages:
    bar_values = get_thread_bar_values(stage_mflop_array[stage])
    hatch_idx = 0
    label = stage

    for values in bar_values:
      plt.bar(
        y_pos, values, bar_width, color=colors[color_id], align='center',
        alpha=0.5, label=label, hatch=hatches[hatch_idx], bottom=previous_values)

      label = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

    color_id += 1

  plt.xticks(y_pos, sched_labels)
  plt.ylabel("MFLOP")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)

  #if img_size == '4K':
  #  plt.ylim(top=100)

  #else:
  #  plt.ylim(top=500)

  plt.legend()
  plt.tight_layout()

  fig.savefig("pdf/flop_per_schedule_{}.pdf".format(img_size), bbox_inches = 'tight', pad_inches = 0)

  fig = plt.figure()

  color_id = 0
  previous_values = [0, 0, 0, 0]

  for stage in stages:
    bar_values = get_thread_bar_values(stage_data_volume_array[stage])
    hatch_idx = 0
    label = stage

    for values in bar_values:
      plt.bar(
        y_pos, values, bar_width, color=colors[color_id], align='center',
        alpha=0.5, label=label, hatch=hatches[hatch_idx], bottom=previous_values)

      label = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

    color_id += 1

  plt.xticks(y_pos, sched_labels)
  plt.ylabel("L3 data volume (MB)")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()
  plt.tight_layout()

  fig.savefig("pdf/data_volume_per_schedule_{}.pdf".format(img_size), bbox_inches = 'tight', pad_inches = 0)
