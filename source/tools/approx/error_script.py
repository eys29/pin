#!/usr/bin/env python

import subprocess
import os
import string
import sys
import math

print(sys.argv)
if len(sys.argv) != 3:
   print("args: <precise> <approx>")
   sys.exit()

PRECISE_FILENAME = sys.argv[1]
APPROX_FILENAME = sys.argv[2]

precise_vals = []
approx_vals = []

with open(PRECISE_FILENAME) as f:
   for line in f:
      if "%" not in line:
         vals = []
         for val in line.split():
            vals.append(float(val))
         precise_vals.append(vals)

with open(APPROX_FILENAME) as f:
   for line in f:
      if (("%" not in line) and ("#" not in line)):
         vals = []
         for val in line.split():
            if val == "NA":
                val = 0
            vals.append(float(val))
         approx_vals.append(vals)

avg_diff = 0.0
max_diff = 0.0
avg_diff_percent00 = 0.0
avg_diff_percent01 = 0.0
avg_diff_percent05 = 0.0
avg_diff_percent10 = 0.0
avg_count = 0.0
snr_num = 0.0
snr_den = 0.0
rmse = 0.0
rmse_count = 0

for i in range(len(precise_vals)):
   for j in range(len(precise_vals[i])):
      diff = abs(precise_vals[i][j] - approx_vals[i][j])

      avg_diff += diff
      if diff > max_diff:
         max_diff = diff
      if diff > (0.0):
         avg_diff_percent00 += 1.0
      if diff > (255.0 * 0.01):
         avg_diff_percent01 += 1.0
      if diff > (255.0 * 0.05):
         avg_diff_percent05 += 1.0
      if diff > (255.0 * 0.1):
         avg_diff_percent10 += 1.0
      avg_count += 1.0
      snr_num += precise_vals[i][j] * precise_vals[i][j]
      snr_den += diff * diff
      rmse += diff * diff
      rmse_count += 1

avg_diff /= avg_count

norm_diff = avg_diff / 255.0
norm_max_diff = max_diff / 255.0
norm_diff_percent00 = avg_diff_percent00 / avg_count
norm_diff_percent01 = avg_diff_percent01 / avg_count
norm_diff_percent05 = avg_diff_percent05 / avg_count
norm_diff_percent10 = avg_diff_percent10 / avg_count

# print "mean pixel diff (normalized to 255):        " + str(norm_diff)
# print "max pixel diff (normalized to 255):         " + str(norm_max_diff)
# print "fraction pixels with diff > 0% (of 255):    " + str(norm_diff_percent00)
# print "fraction pixels with diff > 1% (of 255):    " + str(norm_diff_percent01)
# print "fraction pixels with diff > 5% (of 255):    " + str(norm_diff_percent05)
# print "fraction pixels with diff > 10% (of 255):   " + str(norm_diff_percent10)
rmse = math.sqrt(rmse / rmse_count)
nrmse = rmse / (255.0 - 0.0)
# print "nrmse:                                      " + str(nrmse)
snr = 0.0

if snr_den > 0.0:
   snr = 10 * math.log(snr_num / snr_den, 10)
   # print "signal-to-noise ratio (dB):                 " + str(snr)
   print(snr)
else:   # print "signal-to-noise ratio (dB):                 " + "inf"
   print("inf")

