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

algorithms = ['blur']
hostnames = ['i35']
image_sizes = ['4K', '10K']
schedules = ['breadth_first', 'full_fusion', 'sliding_window', 'tile_32x32']

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

for i in range(len(schedules), counter):
  y_pos = np.arange(len(schedules))

  time_array = []

  for j in range(i, i + len(schedules)):
    time_array.append(time_results[j]['time_data']['all'][0] / time_results[j]['time_iter']['all'][0])

  print(time_array)

  plt.bar(y_pos, time_array, align='center', alpha=0.5)
  plt.xticks(y_pos, schedules)
  plt.ylabel("Schedule")
  plt.title("Execution time per schedule (ms) for a {} image".format(time_results[j]['image_size']))
  plt.show()
