#Copyright (2016) Sandia Corporation
#Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
#the U.S. Government retains certain rights in this software.
import numpy as np
import math

#formatting output of shht for the rh calculator
def vectorize (li):
   vec = []
   curr = 0
   for item in li:
      n = item[0]
      for val in range (curr, n):
         vec.append(item[1])
      curr = n
   vec.append(li[-1][1])
   return vec

#old way of calculating RH distance, very slow
def rh_distance_verify (estimate, actual, eps):
   if eps == 0:
      for i in range (0, max(len(actual), len(estimate))):
         if i >= len(actual) or i >= len(estimate):
            actual.append(0)
            estimate.append(0)
         if actual[i] != estimate[i]:
            return False
      return True

   for i in range (1, len(actual) + 1):
      found = False
      for j in range (int(math.ceil( (1 - eps) * i)), int(math.ceil( (1 + eps) * i)), ):
         if j >= len(estimate):
            estimate.append(0)
         diff = abs((estimate[j-1] - actual[i-1]))
         if eps * actual[i-1] >= diff:
            found = True
            break
      if not found:
         return False

   return True

#part of that old way
def rh_distance_find (estimate, actual):
   if not rh_distance_verify (estimate, actual, 1):
      thing = list (range (10, 51))
   else:
      thing = list (range (0, 11))
   if rh_distance_verify(estimate, actual, 0):
      return 0;
   for i in thing:
      if rh_distance_verify (estimate, actual, i/float(10)):
         for j in range (1, 10):
            if not rh_distance_verify (estimate, actual, i/float(10) - j/float(100)):
               return i/float(10) - (j-1)/float(100)
         return (i-1)/float(10)

#way to calculate RH distance for discrete distributions
def smart_find (F, G):
   F.append(0)
   G.append(0)
   pointwise = [0 for item in F]
   for x in range(1, len(F) + 1):
      if (F[x-1] == 0):
         continue
      min_left = np.inf
      x0 = min(x, len(G))
      while (True):
         eps = min_eps (x, F[x-1], x0, G[x0-1])
         if eps > min_left:
            break
         min_left = eps
         x0 -= 1
         if x0 <= 0:
            break
      min_right = np.inf
      x0 = x
      while (True):
         if x0 > len(G):
            break
         eps = min_eps (x, F[x-1], x0, G[x0-1])
         if eps > min_right:
            break
         min_right = eps
         x0 += 1
      pointwise[x-1] = min (min_left, min_right)
   return max (pointwise)   

def min_eps (x, y, x0, y0):
   yval = 0
   if y0 == 0:
      if y!= 0:
         yval = 1
      else:
         yval = 0 
   else:
      yval = abs(y - y0)/y
   return max( abs(x - x0) /x, yval)

#Shweta, you wanna use this one. It assumes F and G are lists where 
#element in position i is the y-coordinate of your distribution corresponding to an x coordinate with the same value
def continuous_RH (F, G):
   closest_distance = []
   #This might be some hacky stuff to fix some weird corner cases, I don't remember
   G = [G[0]]+ G + [0, 0]
   F = [F[0]] + F
   for x in range(1, len(F)):
      curr = x
      if x >= len(G):
         curr = len(G) - 1
      if G[curr] == F[x]:
         closest_distance.append(0)
         continue
      max_eps = abs(F[x] - G[curr]) / F[x]
      i = 1
      eps = 1 - (x-i)/x
      candidates = [max_eps]
      while True:
         left = round((1 - eps) * x)
         if left < 0:
            left = 0
         right = round((1 + eps) * x)
         if right >= (len(G) - 1):
            right = len(G) - 2
         left_segment = (left, left + 1)
         if left >= len(G) - 1:
            left_segment = (len(G) - 2, len(G) - 1)
         right_segment = (right - 1, right)
         for segment in (left_segment, right_segment):
            endpoint = np.inf
            if segment == left_segment:
               endpoint = segment[0]
            if segment == right_segment:
               endpoint = segment[1]
            candidates.append(max([abs(F[x] - G[endpoint])/F[x], eps]))
            #if (abs(F[x] - G[endpoint])/F[x]) <= eps:
               #found_closest = True
               #min_dist = min ([eps, min_dist])
            #check midpoints of line segment
            try:
               m = (G[segment[0]] - G[segment[1]]) / (segment[0] - segment[1])
            except IndexError:
               print("Segment is " + str(segment) + ", length of G is " + str(len(G)))
               if segment == right_segment:
                  print("Segment is right segment.")
               else:
                  print("Segment is left segment.")
               return -1
            b = G[segment[0]] - (m * segment[0])
            soln = [np.inf, np.inf]
            if (F[x] != x * m):
               soln[0] = (b * x) / (F[x] - x * m)
            if (F[x] != -x* m):
               soln[1] = (2 * x * F[x] - b * x) / (m*x + F[x])
            for s in soln:
               if s == np.inf:
                  continue
               if s < segment[0] or s > segment[1]:
                  continue
               candidates.append(abs(s/x - 1)) 
         i += 1
         eps = 1 - (x-i)/x
         if eps >= max_eps:
            break
      closest_distance.append(min(candidates))
   ret_val = max(closest_distance)
   return max(closest_distance)
