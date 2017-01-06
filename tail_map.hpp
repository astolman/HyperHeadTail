//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#ifndef TAIL_MAP_H
#define TAIL_MAP_H

#include "shll/shll.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <deque>
#include <functional>
#include <algorithm>

typedef std::pair<uint64_t,uint64_t> u64pair;

//abstract class for the items in the S_t hash
class tail_info{
   public:
      virtual bool update(uint32_t timestamp, uint64_t hash, long double p_val) = 0;
      virtual uint64_t estimate(uint32_t window, uint32_t timestamp) = 0;
      uint32_t get_time_added() {
         return time_added;
      }
      virtual ~tail_info() {};
      virtual bool probe(uint32_t timestamp) {};
      int get_size() {
         return counter.get_size();
      }
   protected:
      uint32_t time_added;
      libcount::SHLL counter;
      tail_info(uint32_t t, uint32_t mw, int precision):
         time_added(t), counter(precision, mw){};
};

//class for when p_t is a constant value
class simple_info : public tail_info{
   public:
      simple_info(int precision, uint32_t mw, uint32_t time);
      ~simple_info() {};
      static void set_pt(long double p);
      bool update(uint32_t timestamp, uint64_t hash, long double p_val);
      uint64_t estimate(uint32_t window, uint32_t timestamp);
      bool probe(uint32_t timestamp);
   protected:
      static long double pt;
      uint32_t last_hashed;
};
      
//class for when p_t is a function of time
class fancy_info : public tail_info{
   public:
      fancy_info(int precision, uint32_t mw, uint32_t time);
      ~fancy_info() {};
      static void set_pt(std::function<long double(uint32_t)> fun);
      bool update(uint32_t timestamp, uint64_t hash, long double p_val);
      uint64_t estimate(uint32_t window, uint32_t timestamp);
      static long double call_pt(uint32_t time);
      bool probe(uint32_t timestamp);
   protected:
      static std::function<long double(uint32_t)> pt;
      std::deque<std::pair<uint32_t, long double> > validity_list;
};

class tail_map {
public:
   tail_map (long double pt, uint32_t W, uint8_t precision);
   tail_map (std::function<long double (uint32_t)> fun, uint32_t W, uint8_t precision);
   ~tail_map();
   //update the map after having seen the edge (u,v);
   void update (uint64_t u, uint64_t v, uint32_t timestamp);
   //estimate function returns a vector of pairs where the
   //first item is the degree, and the second is an estimate of 
   //the total number of nodes at that degree
   std::vector<u64pair> estimate (uint32_t timestamp, uint32_t window);
   long double get_pt ();
   uint32_t get_cutoff();
   uint32_t size();
   std::vector<std::pair<int, int> > get_size();
private:
   //this is the map from node labels to a pair which keeps track
   //of the timestamp at which they were added and the SHLL object
   //which keeps track of the degree
   std::unordered_map<uint64_t, std::unique_ptr<tail_info> > map;
   uint32_t counter;
   uint32_t reset_count;
   bool fancy;
   uint32_t max_window;
   long double p;
   uint8_t m;
   void cleanup(uint32_t timestamp);
};

#endif
