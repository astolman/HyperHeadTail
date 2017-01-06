//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#include "tail_map.hpp"
#include "math.h"
#include <iostream>
#include "assert.h"
#include <cassert>
using namespace std;

//~~~~~~~~CLASS TABLE_MAP STUFF~~~~~~~~~~~~~~~~~~~~~~~
tail_map::tail_map (long double pt, uint32_t W, uint8_t precision ):
p(pt), max_window(W), m(precision), fancy(false) {
   //map = unordered_map<uint64_t, simple_info>();
   simple_info::set_pt(pt);
   counter = 0;
}

tail_map::tail_map (std::function<long double (uint32_t)> fun , uint32_t W, uint8_t precision):
max_window(W), m(precision), fancy(true) {
   fancy_info::set_pt(fun);
   p = fancy_info::call_pt(0);
   counter = 0;

   reset_count = uint32_t(W * fancy_info::call_pt(W/2));
}


tail_map::~tail_map() {
/*   for (auto it = map.begin(); it != map.end(); it++)
      delete((it->second).counter);
*/
}

uint32_t tail_map::size() {
   return map.size();
}

void tail_map::update(uint64_t u, uint64_t v, uint32_t timestamp) {
   long double h = static_cast<long double>(libcount::hash(
           libcount::hash(u) ^ v)) / 0xffffffffffffffff;
   if (map.find(u) == map.end()) {
      if (h >= p) 
         return;
      if (! fancy) 
         map.emplace(u, unique_ptr<simple_info>(new simple_info(m, max_window, timestamp)));
      else
         map.emplace(u, unique_ptr<fancy_info>(new fancy_info(m, max_window, timestamp)));
      map.at(u)->update(timestamp, libcount::hash(v), h);
   } else {
      if (map.at(u)->update(timestamp, libcount::hash(v), h)) {
         map.erase(u);
      }
   }
   if (fancy) {
      counter++;
      if (counter >= .95*map.size()) {
         cleanup(timestamp);
         counter = 0;
      }
   }
}

//helper function to compute l(r) for estimate
inline int64_t l (long double p, uint64_t r) {
   return ceil ( (1 - p - pow(1-p, r + 1) - r * p * pow (1-p, r)) / 
                    p * ( 1 - pow( 1 - p , r)));
}

vector<u64pair> tail_map::estimate(uint32_t timestamp, uint32_t window) {
   //first pass, collect the counts and divide them between new arrivals and old
   unordered_map<uint64_t, uint64_t> old_nodes, new_arrivals;
   for (auto it = map.begin(); it != map.end(); it++) {
      uint64_t estimate = it->second->estimate(window, timestamp);
      if (it->second->get_time_added() + window < timestamp) {
         if (old_nodes.find(estimate) == old_nodes.end())
            old_nodes[estimate] = 1;
         else old_nodes[estimate]++;
      } else {
         if (new_arrivals.find(estimate) == new_arrivals.end())
            new_arrivals[estimate] = 1;
         else new_arrivals[estimate]++;
      }
   }


   //now add all the truncated geometric stuff back into new_arrivals
   unordered_map<uint64_t,uint64_t> gt;
   for (auto it = new_arrivals.begin(); it != new_arrivals.end(); it++) {
      uint64_t r = it->first;
      gt[r] = it->second / (1 - pow((1 - p), r));
   }
   //add the old timers back in
   for (auto it = old_nodes.begin(); it != old_nodes.end(); it++) {
      if (it->first == 0) continue;
      if (gt.find(it->first) == gt.end()) gt[it->first] = it->second;
      else gt[it->first] += it->second;
   }
   //put them into the vector and return it
   //this step feels like it could be streamlined...
   vector<u64pair> histogram;
   histogram.reserve(gt.size());
   for (auto it = gt.begin(); it != gt.end(); it++) {
      histogram.push_back(u64pair(it->first, it->second));
   }
   std::sort(histogram.begin(), histogram.end(), 
      [] (u64pair x, u64pair y) {
         return x.first < y.first;
      });
   return histogram;
}

long double tail_map::get_pt () { return p; }

std::vector< std::pair<int, int> > tail_map::get_size() {
   unordered_map<int, int> hist;
   for (auto it = map.begin(); it != map.end(); ++it) {
      int num_entries = it->second->get_size();
      if (hist.find(num_entries) == hist.end())
         hist[num_entries] = 1;
      else
         hist[num_entries] += 1;
   }
   vector<pair<int, int> > ret_vec;
   for (auto it = hist.begin(); it != hist.end(); it++)
      ret_vec.push_back(pair<int, int>(it->first, it->second));
   std::sort(ret_vec.begin(), ret_vec.end(), [] (pair<int, int> x, pair<int, int> y)
      {
         return x.first < y.first;
      });
   return ret_vec;
}

//~~~~~~~~~SIMPLE_INFO STUFF
long double simple_info::pt;

bool simple_info::update(uint32_t timestamp, uint64_t hash, long double p_val) {
   counter.Update(timestamp, hash);
   if (p_val < pt){
      last_hashed = timestamp;
      return false;
   }
   else if (timestamp > last_hashed + counter.get_max_window()) {
      return true;
   }
   else
      return false;
}

uint64_t simple_info::estimate (uint32_t window, uint32_t timestamp) {
   uint64_t estimate = counter.Estimate(window, timestamp);
   if (time_added + window < timestamp)
      return estimate;
   else
      return estimate + l(pt, estimate);
}

void simple_info::set_pt(long double p) {
   pt = p;
}

simple_info::simple_info(int precision, uint32_t mw, uint32_t time): 
tail_info(time, mw, precision){
   /*counter(precision, mw);
   time_added = time;
   */
}

//this is a dummy function, it should never be called
bool simple_info::probe(uint32_t timestamp) {
   assert (1 == 0);
   return false;
}

//FANCY_INFO STUFF~~~~~~~~~~~~~~~~~~
function<long double(uint32_t)> fancy_info::pt;

fancy_info::fancy_info(int precision, uint32_t mw, uint32_t time):
   tail_info(time, mw, precision) {}

void fancy_info::set_pt( std::function<long double(uint32_t)> fun) {
   pt = fun;
}

bool fancy_info::update(uint32_t timestamp, uint64_t hash, long double p_val) {
   counter.Update(timestamp, hash);
   validity_list.push_front(pair<uint32_t, long double>(timestamp, p_val));
   return validity_list.empty();
}

bool fancy_info::probe(uint32_t timestamp) {
   long double prev_hash = 1;
   for (auto it = validity_list.begin(); it != validity_list.end(); it++) {
      it->second;
      pt(timestamp-it->first);
      if ((prev_hash - it->second) < -0.000001 || pt(timestamp - it->first) - it->second < -0.000001){
         it = validity_list.erase(it);
         --it;
      }
      else prev_hash = it->second;
   }
   return validity_list.empty();
}

void tail_map::cleanup(uint32_t timestamp) {
   for (auto it = map.begin(); it != map.end();) {
      if (it->second->probe(timestamp))
         it = map.erase(it);
      else
         ++it;
   }
}

uint64_t fancy_info::estimate(uint32_t window, uint32_t timestamp) {
   long double thresh = pt(window);
   bool valid = false;
   uint32_t time_for_this_pt;
   for (auto it = validity_list.begin(); it != validity_list.end(); it++) {
      if (it->second - thresh < .0000001){
         valid = true;
         time_for_this_pt = it->first;
         if (it->first + window < timestamp) 
            return counter.Estimate(window, timestamp);
      }
   }
   if (valid) {
      //time_added = time_for_this_pt;
      uint64_t est = counter.Estimate(window, timestamp);
      if (time_added + window < timestamp)
         return est;
      else
         return est + l(pt(0), est); 
   }
   else return 0;
}

long double fancy_info::call_pt(uint32_t time) {
   return pt(time);
}
