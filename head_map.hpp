//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#ifndef HEAD_MAP_H
#define HEAD_MAP_H

#include "shll/shll.h"
#include <unordered_map>
#include <vector>
#include <memory>
#include <functional>
#include <iostream>

class head_map {
   public:
      virtual void update (uint64_t u, uint64_t v, uint32_t timestamp) = 0;
      virtual std::vector<std::pair<uint64_t, uint64_t> > estimate
        (uint32_t timestamp, uint32_t window) = 0;
      virtual uint32_t size() = 0;
      virtual long double get_ph(uint32_t timestamp) = 0;
      virtual ~head_map() {};
      virtual std::vector<std::pair<int, int> > get_size() = 0;
};

class simple_head : public head_map{
public:
   simple_head (long double ph, uint32_t W, uint8_t precision);
   ~simple_head();
   uint32_t size();
   //update the map after having seen the edge (u, v)
   void update (uint64_t u, uint64_t v, uint32_t timestamp);
   //return a vector of pairs where the first item is the degree
   //and the second item is an estimate of the total number nodes
   //of that degree
   std::vector<std::pair<uint64_t, uint64_t> > estimate
        (uint32_t timestamp, uint32_t window);
   long double get_ph(uint32_t timestamp);  
   std::vector<std::pair<int, int> > get_size();
private:
   std::unordered_map<uint64_t,libcount::SHLL> map;
   uint32_t max_window;
   long double p;
   uint8_t m;
};

struct head_info {
   long double hashval;
   uint32_t last_hashed;
   libcount::SHLL counter;
   head_info(long double h, int m, uint32_t mw): hashval(h), counter(m, mw) {};
};

class fancy_head : public head_map {
   public:
      fancy_head (std::function<long double(uint32_t)> f, uint32_t W, uint8_t precision);
      uint32_t size();
      void update (uint64_t u, uint64_t v, uint32_t timestamp);
      std::vector<std::pair<uint64_t, uint64_t> > estimate
        (uint32_t timestamp, uint32_t window);
      long double get_ph(uint32_t);
      std::vector<std::pair<int, int> > get_size();
   private:
      std::unordered_map<uint64_t, head_info> map;
      std::function<long double (uint32_t)> p;
      const long double thresh;
      uint32_t max_window;
      uint8_t m;
      uint32_t edge_count;
      uint32_t small_hash_count;
      const long double small_hash;
      void clean(uint32_t timestamp);
};

#endif
