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

  return {
    'image_size': image_size,
    'schedule': schedule,
    'time_data': time_data,
    'time_iter': time_iter,
    'profile_data': profile_data,
    'profile_iter': profile_iter
  }

def print_results(results):
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

algorithms = ['blur']
hostnames = ['i35']
image_sizes = ['4K', '10K']
schedules = ['breadth_first', 'full_fusion', 'sliding_window', 'tile_32x32']
blur_x_stages = ['blur_x_prod']
blur_y_stages = ['blur_y_prod', 'blur_x_cons', 'blur_y_overhead', 'blur_y.s0.c_t0']

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
isizes = len(image_sizes)

for i in range(0, counter, isizes * scheds):
  y_pos = np.arange(scheds)
  bar_width = 0.35

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

  for j in range(isizes):
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
      tdata = time_results[i + j * scheds + k]['time_data']
      titer = time_results[i + j * scheds + k]['time_iter']
      pdata = profile_results[i + j * scheds + k]['profile_data']
      piter = profile_results[i + j * scheds + k]['profile_iter']

      blur_x_time_avg = 0
      blur_x_cache_miss_avg = 0
      blur_x_flop_avg = 0
      blur_x_lines_out_avg = 0
      blur_x_rqsts_miss_avg = 0

      blur_y_time_avg = 0
      blur_y_cache_miss_avg = 0
      blur_y_flop_avg = 0
      blur_y_lines_out_avg = 0
      blur_y_rqsts_miss_avg = 0

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
        if stage_match(stage, blur_x_stages):
          blur_x_cache_miss_avg += pdata[stage][0] / piter[stage][0]
          blur_x_flop_avg += pdata[stage][1] / piter[stage][1]
          blur_x_lines_out_avg += pdata[stage][2] / piter[stage][2]
          blur_x_rqsts_miss_avg += pdata[stage][3] / piter[stage][3]

        if stage_match(stage, blur_y_stages):
          blur_y_cache_miss_avg += pdata[stage][0] / piter[stage][0]
          blur_y_flop_avg += pdata[stage][1] / piter[stage][1]
          blur_y_lines_out_avg += pdata[stage][2] / piter[stage][2]
          blur_y_rqsts_miss_avg += pdata[stage][3] / piter[stage][3]

      blur_x_time_array.append(blur_x_time_avg)
      blur_x_cache_miss_array.append(blur_x_cache_miss_avg)
      blur_x_mflop_array.append(1.E-6 * blur_x_flop_avg)
      blur_x_data_volume_array.append(1.E-6 * (blur_x_lines_out_avg + blur_x_rqsts_miss_avg) * 64)

      blur_y_time_array.append(blur_y_time_avg)
      blur_y_cache_miss_array.append(blur_y_cache_miss_avg)
      blur_y_mflop_array.append(1.E-6 * blur_y_flop_avg)
      blur_y_data_volume_array.append(1.E-6 * (blur_y_lines_out_avg + blur_y_rqsts_miss_avg) * 64)

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

  for k in range(isizes):
    if k == 0:
      label1 = 'blur_x'
      label2 = 'blur_y'

    else:
      label1 = None
      label2 = None

    p1 = plt.bar(
      y_pos + bar_width * k, blur_x_time_mat[k], bar_width, color='#cc0000ff',
      align='center', alpha=0.5, label=label1)

    p2 = plt.bar(
      y_pos + bar_width * k, blur_y_time_mat[k], bar_width, color='#0000ccff',
      align='center', alpha=0.5, label=label2, bottom=blur_x_time_mat[k])

  plt.xticks(y_pos + bar_width * 0.5 * (isizes - 1), schedules)
  plt.xlabel("Schedule")
  plt.ylabel("Time (ms)")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()

  fig.savefig("pdf/time_per_schedule.pdf")

  fig = plt.figure()

  for k in range(isizes):
    if k == 0:
      label1 = 'blur_x'
      label2 = 'blur_y'

    else:
      label1 = None
      label2 = None

    p1 = plt.bar(
      y_pos + bar_width * k, blur_x_cache_miss_mat[k], bar_width, color='#cc0000ff',
      align='center', alpha=0.5, label=label1)

    p2 = plt.bar(
      y_pos + bar_width * k, blur_y_cache_miss_mat[k], bar_width, color='#0000ccff',
      align='center', alpha=0.5, label=label2, bottom=blur_x_cache_miss_mat[k])

  plt.xticks(y_pos + bar_width * 0.5 * (isizes - 1), schedules)
  plt.xlabel("Schedule")
  plt.ylabel("L1 cache misses")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()

  fig.savefig("pdf/cache_miss_per_schedule.pdf")

  fig = plt.figure()

  for k in range(isizes):
    if k == 0:
      label1 = 'blur_x'
      label2 = 'blur_y'

    else:
      label1 = None
      label2 = None

    p1 = plt.bar(
      y_pos + bar_width * k, blur_x_mflop_mat[k], bar_width, color='#cc0000ff',
      align='center', alpha=0.5, label=label1)

    p2 = plt.bar(
      y_pos + bar_width * k, blur_y_mflop_mat[k], bar_width, color='#0000ccff',
      align='center', alpha=0.5, label=label2, bottom=blur_x_mflop_mat[k])

  plt.xticks(y_pos + bar_width * 0.5 * (isizes - 1), schedules)
  plt.xlabel("Schedule")
  plt.ylabel("MFLOP")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()

  fig.savefig("pdf/flop_per_schedule.pdf")

  fig = plt.figure()

  for k in range(isizes):
    if k == 0:
      label1 = 'blur_x'
      label2 = 'blur_y'

    else:
      label1 = None
      label2 = None

    p1 = plt.bar(
      y_pos + bar_width * k, blur_x_data_volume_mat[k], bar_width, color='#cc0000ff',
      align='center', alpha=0.5, label=label1)

    p2 = plt.bar(
      y_pos + bar_width * k, blur_y_data_volume_mat[k], bar_width, color='#0000ccff',
      align='center', alpha=0.5, label=label2, bottom=blur_x_data_volume_mat[k])

  plt.xticks(y_pos + bar_width * 0.5 * (isizes - 1), schedules)
  plt.xlabel("Schedule")
  plt.ylabel("L3 data volume (Mb)")
  plt.axes().yaxis.grid(linestyle=':', linewidth=0.15)
  plt.legend()

  fig.savefig("pdf/data_volume_per_schedule.pdf")
