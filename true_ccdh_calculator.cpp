//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
//This class is for calculating the true ccdh over a given window
//It is not meant to be fast or space-efficient
#include "true_ccdh_calculator.hpp"
#include <iostream>

using namespace std;

true_ccdh_calculator::true_ccdh_calculator() {}

true_ccdh_calculator::~true_ccdh_calculator() {}

void true_ccdh_calculator::add_edge (uint64_t u, uint64_t v, uint32_t timestamp) {
   add_directed(u, v, timestamp);
   add_directed(v, u, timestamp);
}

void true_ccdh_calculator::add_directed (uint64_t u, uint64_t v, uint32_t timestamp) {
   if (map.find(u) == map.end()) {
      unordered_map<uint64_t, uint32_t> new_node;
      new_node[v] = timestamp;
      map[u] = new_node;
   } else 
      map[u][v] = timestamp;
}

//helper function to calculate degree of each vertex
uint64_t degree (unordered_map<uint64_t, uint32_t> node, uint32_t timestamp, uint32_t window) {
   uint64_t deg = 0;
   for (auto it = node.begin(); it != node.end(); it++) 
      if (it->second + window >= timestamp) ++deg;
   return deg;
}

vector<pair<uint64_t,uint64_t> > true_ccdh_calculator::get_ccdh(uint32_t timestamp, uint32_t window) {
   unordered_map<uint64_t, uint64_t> counts;
   for (auto it = map.begin(); it != map.end(); it++) {
      uint64_t deg = degree(it->second, timestamp, window);
      if (deg == 0) continue;
      if (counts.find(deg) == counts.end()) counts[deg] = 1;
      else ++counts[deg];  
   }
   vector<pair<uint64_t,uint64_t> > ccdh;
   ccdh.reserve(counts.size());
   for (auto it = counts.begin(); it != counts.end(); it++)
      ccdh.push_back(pair<uint64_t,uint64_t>(it->first,it->second));
   std::sort(ccdh.begin(), ccdh.end());
   uint64_t sum=0;
   for (auto it = ccdh.rbegin(); it != ccdh.rend(); it++) {
      sum += it->second;
      it->second = sum;
   }
   return ccdh;
}

