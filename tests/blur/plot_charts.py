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
        filename = "{}_{}_profile_{}_{}.csv".format(algorithm, schedule, hostname, image_size)
        profile_results[counter] = get_file_results(filename, True, image_size, schedule)
        print(filename)
        print_results(profile_results[counter])
        print(" ")

        filename = "{}_{}_time_{}_{}.csv".format(algorithm, schedule, hostname, image_size)
        time_results[counter] = get_file_results(filename, False, image_size, schedule)
        print(filename)
        print_results(time_results[counter])
        print(" ")

        counter += 1

for i in range(0, counter, len(schedules)):
  y_pos = np.arange(len(schedules))

  blur_x_time_array = []
  blur_x_cache_miss_array = []
  blur_x_flop_array = []
  blur_x_data_volume_array = []

  blur_y_time_array = []
  blur_y_cache_miss_array = []
  blur_y_flop_array = []
  blur_y_data_volume_array = []

  total_time_array = []
  total_cache_miss_array = []
  total_flop_array = []
  total_data_volume_array = []

  for j in range(len(schedules)):
    tdata = time_results[i + j]['time_data']
    titer = time_results[i + j]['time_iter']
    pdata = profile_results[i + j]['profile_data']
    piter = profile_results[i + j]['profile_iter']

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
    blur_x_flop_array.append(blur_x_flop_avg)
    blur_x_data_volume_array.append(1.E-6 * (blur_x_lines_out_avg + blur_x_rqsts_miss_avg) * 64)

    blur_y_time_array.append(blur_y_time_avg)
    blur_y_cache_miss_array.append(blur_y_cache_miss_avg)
    blur_y_flop_array.append(blur_y_flop_avg)
    blur_y_data_volume_array.append(1.E-6 * (blur_y_lines_out_avg + blur_y_rqsts_miss_avg) * 64)

    total_time_array.append(total_time_avg)
    total_cache_miss_array.append(total_cache_miss_avg)
    total_flop_array.append(total_flop_avg)
    total_data_volume_array.append(1.E-6 * (total_lines_out_avg + total_rqsts_miss_avg) * 64)

  fig = plt.figure()
  p1 = plt.bar(y_pos, blur_x_time_array, align='center', alpha=0.5)
  p2 = plt.bar(y_pos, blur_y_time_array, align='center', alpha=0.5, bottom=blur_x_time_array)
  plt.xticks(y_pos, schedules)
  plt.xlabel("Schedule")
  plt.ylabel("Time (ms)")
  plt.title("Execution time per schedule for {} image".format(time_results[i]['image_size']))
  plt.legend((p1[0], p2[0]), ('blur_x', 'blur_y'))

  bar_counter = 0
  for bar in p2:
    tot = total_time_array[bar_counter]
    plt.text(bar.get_x() + 0.1, tot + 0.5, int(tot))
    bar_counter += 1

  fig.savefig("time_per_schedule_{}.pdf".format(time_results[i]['image_size']))

  fig = plt.figure()
  p1 = plt.bar(y_pos, blur_x_cache_miss_array, align='center', alpha=0.5)
  p2 = plt.bar(y_pos, blur_y_cache_miss_array, align='center', alpha=0.5, bottom=blur_x_cache_miss_array)
  plt.xticks(y_pos, schedules)
  plt.xlabel("Schedule")
  plt.ylabel("Cache misses")
  plt.title("L1 Cache Misses per schedule for {} image".format(time_results[i]['image_size']))
  plt.legend((p1[0], p2[0]), ('blur_x', 'blur_y'))

  bar_counter = 0
  for bar in p2:
    tot = total_cache_miss_array[bar_counter]
    plt.text(bar.get_x() + 0.1, tot + 0.5, int(tot))
    bar_counter += 1

  fig.savefig("cache_miss_per_schedule_{}.pdf".format(time_results[i]['image_size']))

  fig = plt.figure()
  p1 = plt.bar(y_pos, blur_x_flop_array, align='center', alpha=0.5)
  p2 = plt.bar(y_pos, blur_y_flop_array, align='center', alpha=0.5, bottom=blur_x_flop_array)
  plt.xticks(y_pos, schedules)
  plt.xlabel("Schedule")
  plt.ylabel("FLOP")
  plt.title("FLOP per schedule for {} image".format(time_results[i]['image_size']))
  plt.legend((p1[0], p2[0]), ('blur_x', 'blur_y'))

  bar_counter = 0
  for bar in p2:
    tot = total_flop_array[bar_counter]
    plt.text(bar.get_x() + 0.1, tot + 0.5, int(tot))
    bar_counter += 1

  fig.savefig("flop_per_schedule_{}.pdf".format(time_results[i]['image_size']))

  fig = plt.figure()
  p1 = plt.bar(y_pos, blur_x_data_volume_array, align='center', alpha=0.5)
  p2 = plt.bar(y_pos, blur_y_data_volume_array, align='center', alpha=0.5, bottom=blur_x_data_volume_array)
  plt.xticks(y_pos, schedules)
  plt.xlabel("Schedule")
  plt.ylabel("Data volume (Mb)")
  plt.title("L3 Data Volume per schedule for {} image".format(time_results[i]['image_size']))
  plt.legend((p1[0], p2[0]), ('blur_x', 'blur_y'))

  bar_counter = 0
  for bar in p2:
    tot = total_data_volume_array[bar_counter]
    plt.text(bar.get_x() + 0.1, tot + 0.5, int(tot))
    bar_counter += 1

  fig.savefig("data_volume_per_schedule_{}.pdf".format(time_results[i]['image_size']))
