//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#ifndef COUNT_EMPIRICAL_DATA_H_
#define COUNT_EMPIRICAL_DATA_H_

extern const double THRESHOLD_DATA[19];
extern const double RAW_ESTIMATE_DATA[15][201];
extern const double BIAS_DATA[15][201];

// Return the empirical alpha value used for scaling harmonic means.
extern long double EMP_alpha(int precision);

// Return the cardinality threshold for the given precision value.
// Valid values for precision are [4..18] inclusive.
extern double EMP_threshold(int precision);

// Return the empirical bias value for the raw estimate and precision.
extern double EMP_bias(double raw_estimate, int precision);

#endif  // COUNT_EMPIRICAL_DATA_H_
