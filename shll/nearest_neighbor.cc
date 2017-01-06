//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#include "nearest_neighbor.h"
#include <math.h>
#include <set>

namespace libcount {

// Structure used internally to track a "neighbor."
struct Neighbor {
  Neighbor(size_t ind, double diff) : index(ind), difference(diff) {}
  size_t index;
  double difference;
};

// Comparator functor to compare two neighbors. Please note: this is
// purposely implemented to sort elements from greatest-to-least,
// instead of least-to-greatest, which is what comparators normally
// do. This arrangement allows us to remove more distant candidates
// from the set simply by removing the element pointed to by the
// begin() iterator.
class CompareNeighbors {
 public:
  // Compare two neighbors (swapped lhs, rhs order per above comment.)
  bool operator()(const Neighbor& rhs, const Neighbor& lhs) {
    if (lhs.difference < rhs.difference) {
      return true;
    } else if (lhs.difference > rhs.difference) {
      return false;
    } else {
      return lhs.index < rhs.index;
    }
  }
};

size_t NearestNeighbors(const double* array, size_t array_length,
                        const double probe_value, size_t N,
                        size_t* neighbor_indices) {
  // Use an ordered set to track the neighbors with smallest distance
  // from the probe value.
  std::set<Neighbor, CompareNeighbors> neighbors;

  // Scan the input array for potential closest neighbors. The set
  // remains ordered from farthest neighbor to nearest neighbor to
  // make it simple to trim far neighbors while processing.
  for (size_t i = 0; i < array_length; ++i) {
    // Insert a candidate neighbor into the reverse-sorted set.
    Neighbor n(i, fabs(array[i] - probe_value));
    neighbors.insert(n);

    // If we exceeed N, remove the most distant candidate, saving space.
    if (neighbors.size() > N) {
      neighbors.erase(neighbors.begin());
    }
  }

  // Copy the indices of nearest neighbors to the caller's array.
  // We copy in reverse order so that the caller's index array is
  // sorted from nearest neighbor to more distant.
  std::set<Neighbor>::reverse_iterator ri = neighbors.rbegin();
  std::set<Neighbor>::reverse_iterator re = neighbors.rend();
  for (size_t k = 0; ri != re; ++ri, ++k) {
    neighbor_indices[k] = ri->index;
  }

  // Return the number of neighbors found, which may be less than N
  // if array_length is smaller than N.
  return neighbors.size();
}

}  // namespace libcount
