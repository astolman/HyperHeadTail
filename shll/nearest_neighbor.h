//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#ifndef COUNT_NEAREST_NEIGHBOR_H_
#define COUNT_NEAREST_NEIGHBOR_H_

#include <stdlib.h>

namespace libcount {

// Scan an array of doubles, populating the 'neighbor_indices' array with
// at most N array indices of values closest in value to the probe value.
// Returns the actual number of neighbors found, which may be smaller than
// N if 'array_length' is smaller than N.
size_t NearestNeighbors(const double* array, size_t array_length,
                        const double probe_value, size_t N,
                        size_t* neighbor_indices);

}  // namespace libcount

#endif  // COUNT_NEAREST_NEIGHBOR
