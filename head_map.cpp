//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
//tail_map.cpp - implementation for class to hold the hash tables for the
//tail and head sets
#include "head_map.hpp"
#include "assert.h"
#include <iostream>
#include <algorithm>

using namespace std;

const uint64_t base = 0xffffffffffffffff;

//helper function to hash elements to [0, 1]
inline long double interval_hash (uint64_t x) {
   return static_cast<long double> (libcount::hash(x)) / base;
}

simple_head::simple_head (long double ph, uint32_t W, uint8_t precision): p(ph), max_window(W), m(precision) {
   assert(precision >= libcount::HLL_MIN_PRECISION);
   assert(precision <= libcount::HLL_MAX_PRECISION);
   assert (p <= 1 && p >= 0);
}

simple_head::~simple_head() {
}

uint32_t simple_head::size() {
   return map.size();
}

void simple_head::update (uint64_t u, uint64_t v, uint32_t timestamp) {
   long double h = interval_hash (u);
   if ( h > p) return;
   if (map.find(u) == map.end())
      map.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(m, max_window));
   map.at(u).Update (timestamp, libcount::hash(v));
}

vector<pair<uint64_t,uint64_t> > simple_head::estimate(uint32_t timestamp, uint32_t window) {
   //first pass over the table to collect counts of nodes at whatever degrees
   unordered_map<uint64_t, uint64_t> degree_to_count_map;
   for (auto it = map.begin(); it != map.end(); it++) {
      uint64_t degree = it->second.Estimate(window, timestamp);
      if (degree == 0) continue;
      if (degree_to_count_map.find(degree) == degree_to_count_map.end())
         degree_to_count_map[degree] = 1;
      else degree_to_count_map[degree]++;
   }

   //now pass over the degree_to_map and scale the sizes and add them to the vector
   //we return
   vector<pair<uint64_t, uint64_t> > histogram;
   histogram.reserve(degree_to_count_map.size());
   for (auto it = degree_to_count_map.begin(); it != degree_to_count_map.end(); it++)
      histogram.push_back(pair<uint64_t,uint64_t>(it->first, it->second / p));
   //sort the vector by degree
   std::sort(histogram.begin(), histogram.end(), 
      [](pair<uint64_t,uint64_t> x, pair<uint64_t,uint64_t> y) {
         return x.first < y.first;});
   return histogram;
}

long double simple_head::get_ph (uint32_t timestamp) { return p; }

vector<pair<int, int> > simple_head::get_size() {
   unordered_map<int, int> hist;
   for (auto it = map.begin(); it != map.end(); ++it) {
      int num_ents = it->second.get_size();
      if (hist.find(num_ents) == hist.end())
         hist[num_ents] = 1;
      else
         hist[num_ents] += 1;
   }
   vector<pair<int, int> > vec;
   for (auto it = hist.begin(); it != hist.end(); ++it) {
      vec.push_back(pair<int, int>(it->first, it->second));
   }

   std::sort(vec.begin(), vec.end(), 
      [](pair<int,int> x, pair<int,int> y) {
         return x.first < y.first;});
   return vec;
}
//~~~~~~COMMENCE THE FANCINESS!!!!!!!!
fancy_head::fancy_head(std::function<long double(uint32_t)>f, uint32_t W, uint8_t precision):
p(f), max_window(W), m(precision), edge_count(0), small_hash_count(0), thresh(p(0)), small_hash(p(max_window)) {
}

uint32_t fancy_head::size() {
   return map.size();
}

void fancy_head::update(uint64_t u, uint64_t v, uint32_t timestamp) {
   long double h = interval_hash (u);
   if (h < small_hash) small_hash_count++;
   if (h - thresh < 0.00000001) {
      if (map.find(u) == map.end()) 
         map.emplace(u, head_info(h, m, max_window));
      else
         map.at(u).counter.Update(timestamp, libcount::hash(v));
      map.at(u).last_hashed = timestamp;
   }
   if (map.find(u) != map.end())
      map.at(u).counter.Update(timestamp, libcount::hash(v));
   edge_count++;
   if (edge_count > static_cast<double>(map.size()) * .95 ){
      clean(timestamp);
   }
}

void fancy_head::clean(uint32_t timestamp) {
   for (auto it = map.begin(); it != map.end();) 
      if (p(timestamp - it->second.last_hashed) < it->second.hashval) {
         it = map.erase(it);
      }
      else
         it++;
   small_hash_count = edge_count = 0;
}

std::vector<std::pair<uint64_t, uint64_t> > fancy_head::estimate (uint32_t timestamp, uint32_t window) {
   unordered_map<uint64_t, uint64_t> degree_to_count_map;
   long double thresh_for_time = p(window);
   for (auto it = map.begin(); it != map.end(); it++) {
      if (it->second.hashval > thresh_for_time) continue;
      uint64_t degree = it->second.counter.Estimate(window, timestamp);
      if (degree == 0) continue;
      if (degree_to_count_map.find(degree) == degree_to_count_map.end())
         degree_to_count_map[degree] = 1;
      else degree_to_count_map[degree]++;
   }

   //now pass over the degree_to_map and scale the sizes and add them to the vector
   //we return
   vector<pair<uint64_t, uint64_t> > histogram;
   histogram.reserve(degree_to_count_map.size());
   for (auto it = degree_to_count_map.begin(); it != degree_to_count_map.end(); it++) {
      histogram.push_back(pair<uint64_t,uint64_t>(it->first, it->second / thresh_for_time));
   }
   //sort the vector by degree
   std::sort(histogram.begin(), histogram.end(), 
      [](pair<uint64_t,uint64_t> x, pair<uint64_t,uint64_t> y) {
         return x.first < y.first;});
   return histogram;
}

long double fancy_head::get_ph(uint32_t timestamp) {
   return p(timestamp);
}

vector<pair<int, int> > fancy_head::get_size() {
   unordered_map<int, int> hist;
   for (auto it = map.begin(); it != map.end(); ++it) {
      int num_ents = it->second.counter.get_size();
      if (hist.find(num_ents) == hist.end())
         hist[num_ents] = 1;
      else
         hist[num_ents] += 1;
   }
   vector<pair<int, int> > vec;
   for (auto it = hist.begin(); it != hist.end(); ++it) {
      vec.push_back(pair<int, int>(it->first, it->second));
   }

   std::sort(vec.begin(), vec.end(), 
      [](pair<int,int> x, pair<int,int> y) {
         return x.first < y.first;});
   return vec;
}
