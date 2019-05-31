import csv

with open('blur_breadth_first_profile_i35_10K.csv') as csvfile:
    csv_data = csv.reader(csvfile, delimiter=',')
    time_results = True
    time_data = {}
    profile_data = {}
    time_iter = {}
    profile_iter = {}

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

      else:
        time_results = not time_results

for stage in time_data:
  time_output = stage

  for data in time_data[stage]:
    time_output += ",{}".format(time_data[stage][data] / time_iter[stage][data])

  print(time_output)

for stage in profile_data:
  profile_output = stage

  for data in profile_data[stage]:
    profile_output += ",{}".format(profile_data[stage][data] / profile_iter[stage][data])

  print(profile_output)

