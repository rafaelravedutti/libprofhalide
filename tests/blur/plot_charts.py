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

def stage_match(stage, stage_list):
  for stg in stage_list:
    if stage.startswith(stg):
      return True

  return False

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

algorithms = ['blur']
hostnames = ['i35']
image_sizes = ['4K', '10K']
schedules = ['breadth_first', 'full_fusion', 'sliding_window', 'tile_32x32']
blur_x_stages = ['blur_x_prod']
blur_y_stages = ['blur_y_prod', 'blur_x_cons', 'blur_y_overhead', 'blur_y.s0.c_t0']
variants = ['serial', 'parallel']

profile_results = {}
time_results = {}
counter = 0

for algorithm in algorithms:
  for hostname in hostnames:
    for image_size in image_sizes:
      for variant in variants:
        for schedule in schedules:
          filename = "csv/{}_{}_{}_profile_{}_{}.csv".format(algorithm, schedule, variant, hostname, image_size)
          profile_results[counter] = get_file_results(filename, True, image_size, schedule)
          print(filename)
          print_results(profile_results[counter])
          print(" ")

          filename = "csv/{}_{}_{}_time_{}_{}.csv".format(algorithm, schedule, variant, hostname, image_size)
          time_results[counter] = get_file_results(filename, False, image_size, schedule)
          print(filename)
          print_results(time_results[counter])
          print(" ")

          counter += 1

scheds = len(schedules)
varnts = len(variants)

plt.rcParams.update({'font.size': 18})
sched_labels = ["1t   4t\n" + sched.replace("_", "\n") for sched in schedules]
sched_labels[2] = sched_labels[2].replace("4t", "   ")
hatches = (' ', '////', '++++', '----')

for i in range(0, counter, scheds * varnts):
  y_pos = np.arange(scheds)
  bar_width = 0.35
  bar_sep = 0.1 

  img_size = None

  blur_x_time_mat = []
  blur_x_cache_miss_mat = []
  blur_x_mflop_mat = []
  blur_x_data_volume_mat = []

  blur_y_time_mat = []
  blur_y_cache_miss_mat = []
  blur_y_mflop_mat = []
  blur_y_data_volume_mat = []

  total_time_mat = []
  total_cache_miss_mat = []
  total_mflop_mat = []
  total_data_volume_mat = []

  for j in range(varnts):
    blur_x_time_array = []
    blur_x_cache_miss_array = []
    blur_x_mflop_array = []
    blur_x_data_volume_array = []

    blur_y_time_array = []
    blur_y_cache_miss_array = []
    blur_y_mflop_array = []
    blur_y_data_volume_array = []

    total_time_array = []
    total_cache_miss_array = []
    total_mflop_array = []
    total_data_volume_array = []

    for k in range(scheds):
      idx = i + j * scheds + k

      if img_size is None:
        img_size = time_results[idx]['image_size']

      blur_x_time_avg = 0
      blur_x_cache_miss_avg = {}
      blur_x_flop_avg = {}
      blur_x_lines_out_avg = {}
      blur_x_rqsts_miss_avg = {}

      blur_y_time_avg = 0
      blur_y_cache_miss_avg = {}
      blur_y_flop_avg = {}
      blur_y_lines_out_avg = {}
      blur_y_rqsts_miss_avg = {}

      total_time_avg = 0
      total_cache_miss_avg = 0
      total_flop_avg = 0
      total_lines_out_avg = 0
      total_rqsts_miss_avg = 0

      if time_results[idx]['time_data'] is not None and profile_results[idx]['profile_data'] is not None:
        tdata = time_results[idx]['time_data']
        titer = time_results[idx]['time_iter']
        pdata = profile_results[idx]['profile_data']
        piter = profile_results[idx]['profile_iter']

        total_time_avg = tdata['all'][0] / titer['all'][0]
        total_cache_miss_avg = pdata['total'][0] / piter['total'][0]
        total_flop_avg = pdata['total'][1] / piter['total'][1]
        total_lines_out_avg = pdata['total'][2] / piter['total'][2]
        total_rqsts_miss_avg = pdata['total'][3] / piter['total'][3]

        for stage in tdata:
          if stage == 'blur_x':
            blur_x_time_avg += tdata[stage][0] / titer[stage][0]

          if stage == 'blur_y':
            blur_y_time_avg += tdata[stage][0] / titer[stage][0]

        for stage in pdata:
          if is_thread_stage(stage):
            thread = stage[-2:]

            if stage_match(stage, blur_x_stages):
              if thread not in blur_x_cache_miss_avg:
                blur_x_cache_miss_avg[thread] = 0
                blur_x_flop_avg[thread] = 0
                blur_x_lines_out_avg[thread] = 0
                blur_x_rqsts_miss_avg[thread] = 0

              blur_x_cache_miss_avg[thread] += pdata[stage][0] / piter[stage][0]
              blur_x_flop_avg[thread] += pdata[stage][1] / piter[stage][1]
              blur_x_lines_out_avg[thread] += pdata[stage][2] / piter[stage][2]
              blur_x_rqsts_miss_avg[thread] += pdata[stage][3] / piter[stage][3]

            if stage_match(stage, blur_y_stages):
              if thread not in blur_y_cache_miss_avg:
                blur_y_cache_miss_avg[thread] = 0
                blur_y_flop_avg[thread] = 0
                blur_y_lines_out_avg[thread] = 0
                blur_y_rqsts_miss_avg[thread] = 0

              blur_y_cache_miss_avg[thread] += pdata[stage][0] / piter[stage][0]
              blur_y_flop_avg[thread] += pdata[stage][1] / piter[stage][1]
              blur_y_lines_out_avg[thread] += pdata[stage][2] / piter[stage][2]
              blur_y_rqsts_miss_avg[thread] += pdata[stage][3] / piter[stage][3]

      blur_x_mflop_avg = {}
      blur_y_mflop_avg = {}
      blur_x_data_volume_avg = {}
      blur_y_data_volume_avg = {}

      for t in blur_x_flop_avg:
        blur_x_mflop_avg[t] = 1.E-6 * blur_x_flop_avg[t]

      for t in blur_y_flop_avg:
        blur_y_mflop_avg[t] = 1.E-6 * blur_y_flop_avg[t]

      for t in blur_x_lines_out_avg:
        blur_x_data_volume_avg[t] = 1.E-6 * (blur_x_lines_out_avg[t] + blur_x_rqsts_miss_avg[t]) * 64

      for t in blur_y_lines_out_avg:
        blur_y_data_volume_avg[t] = 1.E-6 * (blur_y_lines_out_avg[t] + blur_y_rqsts_miss_avg[t]) * 64

      blur_x_time_array.append(blur_x_time_avg)
      blur_x_cache_miss_array.append(blur_x_cache_miss_avg)
      blur_x_mflop_array.append(blur_x_mflop_avg)
      blur_x_data_volume_array.append(blur_x_data_volume_avg)

      blur_y_time_array.append(blur_y_time_avg)
      blur_y_cache_miss_array.append(blur_y_cache_miss_avg)
      blur_y_mflop_array.append(blur_y_mflop_avg)
      blur_y_data_volume_array.append(blur_y_data_volume_avg)

      total_time_array.append(total_time_avg)
      total_cache_miss_array.append(total_cache_miss_avg)
      total_mflop_array.append(1.E-6 * total_flop_avg)
      total_data_volume_array.append(1.E-6 * (total_lines_out_avg + total_rqsts_miss_avg) * 64)

    blur_x_time_mat.append(blur_x_time_array)
    blur_x_cache_miss_mat.append(blur_x_cache_miss_array)
    blur_x_mflop_mat.append(blur_x_mflop_array)
    blur_x_data_volume_mat.append(blur_x_data_volume_array)

    blur_y_time_mat.append(blur_y_time_array)
    blur_y_cache_miss_mat.append(blur_y_cache_miss_array)
    blur_y_mflop_mat.append(blur_y_mflop_array)
    blur_y_data_volume_mat.append(blur_y_data_volume_array)

    total_time_mat.append(total_time_array)
    total_cache_miss_mat.append(total_cache_miss_array)
    total_mflop_mat.append(total_mflop_array)
    total_data_volume_mat.append(total_data_volume_array)

  fig = plt.figure()

  for k in range(varnts):
    if k == 0:
      label1 = 'blur_x'
      label2 = 'blur_y'

    else:
      label1 = None
      label2 = None

    p1 = plt.bar(
      y_pos + bar_width * k, blur_x_time_mat[k], bar_width - bar_sep * 0.5, color='#ff0000ff',
      align='center', alpha=0.5, label=label1)

    p2 = plt.bar(
      y_pos + bar_width * k, blur_y_time_mat[k], bar_width - bar_sep * 0.5, color='#00ff00ff',
      align='center', alpha=0.5, label=label2, bottom=blur_x_time_mat[k])

  plt.xticks(y_pos + bar_width * 0.5 * (varnts - 1), sched_labels)
  plt.ylabel("Time (ms)")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()
  plt.tight_layout()

  fig.savefig("pdf/time_per_schedule_{}.pdf".format(img_size))

  fig = plt.figure()

  for k in range(varnts):
    if k == 0:
      label1 = 'blur_x'
      label2 = 'blur_y'
    else:
      label1 = None
      label2 = None

    previous_values = [0, 0, 0, 0]

    bar_values = get_thread_bar_values(blur_x_cache_miss_mat[k])
    hatch_idx = 0

    for values in bar_values:
      p1 = plt.bar(
        y_pos + bar_width * k, values, bar_width - bar_sep * 0.5, color='#ff0000ff', align='center',
        alpha=0.5, label=label1, hatch=hatches[hatch_idx], bottom=previous_values)

      label1 = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

    bar_values = get_thread_bar_values(blur_y_cache_miss_mat[k])
    hatch_idx = 0

    for values in bar_values:
      p2 = plt.bar(
        y_pos + bar_width * k, values, bar_width - bar_sep * 0.5, color='#00ff00ff', align='center',
        alpha=0.5, label=label2, hatch=hatches[hatch_idx], bottom=previous_values)

      label2 = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

  plt.xticks(y_pos + bar_width * 0.5 * (varnts - 1), sched_labels)
  plt.ylabel("L1 cache misses")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()
  plt.tight_layout()

  fig.savefig("pdf/cache_miss_per_schedule_{}.pdf".format(img_size))

  fig = plt.figure()

  for k in range(varnts):
    if k == 0:
      label1 = 'blur_x'
      label2 = 'blur_y'
    else:
      label1 = None
      label2 = None

    previous_values = [0, 0, 0, 0]

    bar_values = get_thread_bar_values(blur_x_mflop_mat[k])
    hatch_idx = 0

    for values in bar_values:
      p1 = plt.bar(
        y_pos + bar_width * k, values, bar_width - bar_sep * 0.5, color='#ff0000ff', align='center',
        alpha=0.5, label=label1, hatch=hatches[hatch_idx], bottom=previous_values)

      label1 = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

    bar_values = get_thread_bar_values(blur_y_mflop_mat[k])
    hatch_idx = 0

    for values in bar_values:
      p2 = plt.bar(
        y_pos + bar_width * k, values, bar_width - bar_sep * 0.5, color='#00ff00ff', align='center',
        alpha=0.5, label=label2, hatch=hatches[hatch_idx], bottom=previous_values)

      label2 = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

  plt.xticks(y_pos + bar_width * 0.5 * (varnts - 1), sched_labels)
  plt.ylabel("MFLOP")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()
  plt.tight_layout()

  fig.savefig("pdf/flop_per_schedule_{}.pdf".format(img_size))

  fig = plt.figure()

  for k in range(varnts):
    if k == 0:
      label1 = 'blur_x'
      label2 = 'blur_y'
    else:
      label1 = None
      label2 = None

    previous_values = [0, 0, 0, 0]

    bar_values = get_thread_bar_values(blur_x_data_volume_mat[k])
    hatch_idx = 0

    for values in bar_values:
      p1 = plt.bar(
        y_pos + bar_width * k, values, bar_width - bar_sep * 0.5, color='#ff0000ff', align='center',
        alpha=0.5, label=label1, hatch=hatches[hatch_idx], bottom=previous_values)

      label1 = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

    bar_values = get_thread_bar_values(blur_y_data_volume_mat[k])
    hatch_idx = 0

    for values in bar_values:
      p2 = plt.bar(
        y_pos + bar_width * k, values, bar_width - bar_sep * 0.5, color='#00ff00ff', align='center',
        alpha=0.5, label=label2, hatch=hatches[hatch_idx], bottom=previous_values)

      label2 = None
      previous_values = vector_add(previous_values, values)
      hatch_idx += 1

  plt.xticks(y_pos + bar_width * 0.5 * (varnts - 1), sched_labels)
  plt.ylabel("L3 data volume (Mb)")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()
  plt.tight_layout()

  fig.savefig("pdf/data_volume_per_schedule_{}.pdf".format(img_size))
