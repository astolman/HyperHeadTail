#Copyright (2016) Sandia Corporation
#Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#the U.S. Government retains certain rights in this software.
import os
import re
import pandas as pd
import numpy as np
def run_test (graph, ph, pt, max_win, tru, query, output):
   command = '~/Documents/shht-git/shht-implementation/shht '
   command += '--input ' +graph
   command += ' --output ' + output
   command += ' --ph ' + str(ph)
   command += ' --pt ' + str(pt)
   command += ' --max-window ' + str(max_win)
   if tru:
      command += ' --get-true-distribution'
   command += ' < ' + query
   print (command)
   os.system(command)

def generate_input (name, base, step, stop):
   output = (name, 'w')
   while base <= stop:
      output.write(str(base))
      base = step(base)
   output.close()

def graduate_ph (graph, inc, stop, pt, max_win, tru, query):
   ph = 0
   while ph <= stop:
      run_test (graph, ph, pt, max_win, tru, query)
      ph += inc

def graduate_pt (graph, ph, inc, stop, max_win, tru, query):
   pt = 0
   while pt <= stop:
      run_test (graph, ph, pt, max_win, tru, query)
      pt += inc

def clean(foo):
   input = open(foo, 'r')
   true = False
   data = []
   state = 0;
   curr = []
   for line in input:
      exp = re.match('File : (\S+), length = (\d+), Head size = (\d+), Tail size = (\d+) ph = (\d*(\.\d+)?), pt = (\d*(\.\d+)?)', line)
      if exp is not None:
         data.append(curr)
         curr =  [exp.group(1), int(exp.group(2)), int(exp.group(3)), int (exp.group(4)), float(exp.group(5)), float(exp.group(7))]
         state = 1
         continue
      if state == 4:
         exp = re.match('      dthr = (\d+)', line)
         if exp is None:
            state = 1
         else:
            curr.append(int(exp.group(1)))
            state = 1
            continue
      if state == 1:
         if len(curr) > 6:
            data.append(curr)
            curr = curr[:6]
         exp = re.match('   (True distro|Estimate), [Ww]indow = (\d+)', line)
         if exp is None:
            print (line)
         curr += [exp.group(1), int(exp.group(2))]
         state += 1
         continue
      if state == 2:
         exp = re.match('      xs = \[(.+)\]', line)
         if exp is None:
            print (line)
         li = exp.group(1).split(', ')
         li = map(lambda x: int(x), li)
         curr.append(list(li))
         state += 1
         continue
      if state == 3:
         exp = re.match('      ys = \[(.+)\]', line)
         li = exp.group(1).split(', ')
         li = map(lambda x: int(x), li)
         curr.append(list(li))
         state += 1
         continue
   data.append(curr)
   df = pd.DataFrame(data, columns = ['File', 'Size', '|Sh|', '|St|', 'ph', 'pt', 'category', 'window', 'xs', 'ys', 'dthr'])
   return df

