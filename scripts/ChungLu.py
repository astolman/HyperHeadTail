#Copyright (2016) Sandia Corporation
#Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#the U.S. Government retains certain rights in this software.
import random
import bisect
import math
import pickle
import sys
import os
import math

def make_graph(s, n, file):
    if s == 'lognormal':
        generate(n, lambda x: 1 - (1/2 + 1/2*math.erf(math.log(x)/math.sqrt(2))), file)
    else:
        foo = open('/afs/cats.ucsc.edu/users/s/astolman/shht/scripts/zeta%1.1f' %float(s), 'r')
        cdf = [float(x[:-1]) for x in foo]
        foo.close()
        generate(n, lambda x: cdf[x] if x < len(cdf) else len(cdf), file)

def generate (n, p, out):
   #n -- number of nodes
   #p -- cdf
   nodes = [0 for i in range(n)]
   for index,v in enumerate (nodes):
      u = random.random()
      total = 0
      for i in range (1, 1000001):
         try:
            if u <= p(i) and u >= p(i-1):
               nodes[index] = i
               break
         except ValueError:
            continue
   print("Made nodes")
   total = sum(nodes)
   nodes = list(map (lambda x: x /total, nodes))
   s = 0
   intervals = [(0, 0) for i in nodes]
   for i in range (len(nodes)):
      intervals[i] = (s, s + nodes[i])
      s += nodes[i]
   output = open(out, 'w')
   for i in range (total):
      vals = [random.random(), random.random()]
      endpoints = [0, 0]
      for j, u in enumerate(vals):
         curr = math.ceil(len(intervals) / 2)
         left = 0
         right = len(intervals)
         while True:
            bucket = intervals[curr]
            if u >= bucket[0] and u <= bucket[1]:
               endpoints[j] = curr
               break
            if u >= bucket[1]:
               left = curr
               curr = curr + math.ceil((right - curr) / 2)
            else:
               right = curr
               curr = curr - math.ceil(curr - left / 2)
      output.write(str(endpoints[0]) + ' ' + str(endpoints[1]) + ' ' + str(i) + '\n')
   output.close()

words = sys.argv[1].split(' ')
print (str(sys.argv))
command = ' '.join(words[2:])
make_graph(words[0], 1000000, '/afs/cats.ucsc.edu/users/s/astolman/' + words[1])
os.system(command)
