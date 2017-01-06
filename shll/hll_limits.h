//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.

#ifndef INCLUDE_COUNT_HLL_LIMITS_H_
#define INCLUDE_COUNT_HLL_LIMITS_H_

#ifdef __cplusplus
namespace libcount {
#endif

enum {
  /* Minimum and maximum precision values allowed. */
  HLL_MIN_PRECISION = 4,
  HLL_MAX_PRECISION = 18
};

#ifdef __cplusplus
}  // namespace libcount
#endif

#endif  // INCLUDE_COUNT_HLL_LIMITS_H_
