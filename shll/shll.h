//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#ifndef INCLUDE_COUNT_SHLL_H_
#define INCLUDE_COUNT_SHLL_H_

#include <stdint.h>
#include "hll_limits.h"
#include <deque>
#include <memory>
#include <iostream>
#include <vector>

namespace libcount {

class SHLL {
 public:
  ~SHLL();

  // Create an instance of a HyperLogLog++ cardinality estimator. Valid values
  // for precision are [4..18] inclusive, and govern the precision of the
  // estimate. Returns NULL on failure. In the event of failure, the caller
  // may provide a pointer to an integer to learn the reason.
  //static SHLL* Create(int precision, uint32_t max_window, int* error = 0);

  // Update the instance to record the observation of an element. It is
  // assumed that the caller uses a high-quality 64-bit hash function that
  // is free of bias. Empirically, using a subset of bits from a well-known
  // cryptographic hash function such as SHA1, is a good choice.
  void Update(uint32_t timestamp, uint64_t hash);

  // Merge count tracking information from another instance into the object.
  // The object being merged in must have been instantiated with the same
  // precision. Returns 0 on success, EINVAL otherwise.
  //int Merge(const HLL* other);

  // Compute the bias-corrected estimate using the HyperLogLog++ algorithm.
  uint64_t Estimate(uint32_t window, uint32_t timestamp) const;
  int get_size();
  SHLL(int precision, uint32_t max_window);
  uint32_t get_max_window();
 private:
  // No copying allowed
  //SHLL& operator=(const SHLL& no_assign);

  // Constructor is private: we validate the precision in the Create function.

  // Compute the raw estimate based on the HyperLogLog algorithm.
  long double RawEstimate(uint32_t window, uint32_t timestamp) const;

  // Return the number of registers equal to zero; used in LinearCounting.
  uint16_t RegistersEqualToZero(uint32_t window, uint32_t timestamp) const;

  int precision_;
  int register_count_;
  std::shared_ptr<std::vector<std::deque<std::pair<uint32_t,uint8_t> > > >registers_;
  uint32_t max_window_;
};

uint64_t hash(uint64_t i);
}  // namespace libcount

#endif  // INCLUDE_COUNT_SHLL_H_
