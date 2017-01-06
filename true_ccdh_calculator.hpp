//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#include <unordered_map>
#include <vector>
#include <algorithm>

class true_ccdh_calculator {
public:
   true_ccdh_calculator();
   ~true_ccdh_calculator();
   void add_edge (uint64_t u, uint64_t v, uint32_t timestamp);
   std::vector<std::pair<uint64_t,uint64_t> > get_ccdh (uint32_t timestamp, uint32_t window);
   void add_directed(uint64_t u, uint64_t v, uint32_t timestamp);
private:
   std::unordered_map<uint64_t,std::unordered_map<uint64_t,uint32_t> > map;
   
};
