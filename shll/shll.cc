//Copyright (2016) Sandia Corporation
//Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//the U.S. Government retains certain rights in this software.
#include "shll.h"
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include "empirical_data.h"
#include "utility.h"
#include <deque>
#include <iostream>
#include <fstream>
#include <openssl/sha.h>

namespace {
using namespace std;

using libcount::CountLeadingZeroes;
using std::max;

// Helper that calculates cardinality according to LinearCounting
double LinearCounting(double register_count, double zeroed_registers) {
  return register_count * log(register_count / zeroed_registers);
}

// Helper to calculate the index into the table of registers from the hash
inline uint64_t RegisterIndexOf(uint64_t hash, int precision) {
  return (hash >> (64 - precision));
}

// Helper to count the leading zeros (less the bits used for the reg. index)
inline uint8_t ZeroCountOf(uint64_t hash, int precision) {
  // Make a mask for isolating the leading bits used for the register index.
  const uint64_t ONE = 1;
  const uint64_t mask = ~(((ONE << precision) - ONE) << (64 - precision));

  // Count zeroes, less the index bits we're masking off.
  return (CountLeadingZeroes(hash & mask) - static_cast<uint8_t>(precision));
}

}  // namespace

namespace libcount {

SHLL::SHLL(int precision, uint32_t max_window)
    : precision_(precision), max_window_(max_window), register_count_(0), 
        registers_(new std::vector<std::deque<std::pair<uint32_t, uint8_t> > >()) {
  // The precision is vetted by the Create() function.  Assertions nonetheless.
  assert(precision >= HLL_MIN_PRECISION);
  assert(precision <= HLL_MAX_PRECISION);

  // We employ (2 ^ precision) "registers" to store max leading zeroes.
  register_count_ = (1 << precision_);
  registers_.get()->reserve(register_count_);
  for (int i = 0; i < register_count_; i++) 
     registers_.get()->push_back(std::deque<std::pair<uint32_t, uint8_t> > ());
  //registers_ = std::ke_unique<std::deque<pair<uint32_t, uint8_t> >[]>(register_count_);
  // Allocate space for the registers. We can safely economize by using bytes
  // for the counters because we know the value can't ever exceed ~60.
  //registers_ = std::deque<pair<uint32_t,uint8_t> >[register_count_];
  //for (int i = 0; i < register_count_; i) registers_[i] = list<pair<uint32_t,uint8_t> >;
}

   
SHLL::~SHLL() { /*delete[] registers_; */}

/*SHLL* SHLL::Create(int precision, uint32_t max_window, int* error) {
  if ((precision < HLL_MIN_PRECISION) || (precision > HLL_MAX_PRECISION)) {
    MaybeAssign(error, EINVAL);
    return NULL;
  }
  return new SHLL(precision, max_window);
}*/

uint32_t SHLL::get_max_window() {
   return max_window_;
}

//registers hold a list of pairs, for each pair the first item is the
//timestamp at which it arrived, and the second is the index of the first
//nonzero in its hash code
void SHLL::Update(uint32_t timestamp, uint64_t hash) {
  // Which register will potentially receive the zero count of this hash?
  const uint64_t index = RegisterIndexOf(hash, precision_);
  // Count the zeroes for the hash, and add one, per the algorithm spec.
  const uint8_t count = ZeroCountOf(hash, precision_) + 1;
  //debugging stuff
  // Update the appropriate register
  (*registers_.get())[index].push_front(pair<uint32_t,uint8_t>(timestamp, count));
  for (auto it = ++((*registers_.get())[index].begin()),end=(*registers_.get())[index].end(); it != end; it++) {
     if (it->second <= count) {
        auto jt = it;
        for (; jt != (*registers_.get())[index].end(); jt++) {
           if (jt->second > count) break;
        }
        (*registers_.get())[index].erase(it, jt);
        break;
     }
  }
  if (timestamp < max_window_) return;
  auto it = (*registers_.get())[index].rbegin();
  for (; it != (*registers_.get())[index].rend(); it++) 
     if (it->first > timestamp - max_window_) {
        it--;
        break;
     }
  (*registers_.get())[index].erase( --(it.base()), (*registers_.get())[index].end());
}

//this was in the original HLL implementation, don't think I need it though
/*int SHLL::Merge(const SHLL* other) {
  assert(other != NULL);
  if (other == NULL) {
    return EINVAL;
  }

  // Ensure that the precision values of the two objects match.
  if (precision_ != other->precision_) {
    return EINVAL;
  }

  // Choose the maximum of corresponding registers from self, other and
  // store it back in self, effectively merging the state of the counters.
  for (int i = 0; i < register_count_; ++i) {
    registers_[i] = max(registers_[i], other->registers_[i]);
  }

  return 0;
}*/

long double SHLL::RawEstimate(uint32_t window, uint32_t timestamp) const {
  // Let 'm' be the number of registers.
  const double m = static_cast<double>(register_count_);
  /*
  for (int i = 0; i < register_count_; i++) {
     cout << "Register "<<i<<": \n";
     for (auto it = registers_[i].begin(); it != registers_[i].end(); ++it)
        cout << "(" << it->first << ", " << to_string(it->second) << ") ";
     cout << "\n";
  }
  */

  // For each register, let 'max' be the contents of the register.
  // Let 'term' be the reciprocal of 2 ^ max.
  // Finally, let 'sum' be the sum of all terms.
  double sum = 0.0;
  for (int i = 0; i < register_count_; ++i) {
    double max = 0;
    for (auto it = (*registers_.get())[i].begin(); it != (*registers_.get())[i].end(); it++){
       if (it->first + window < timestamp) 
          break;
       max = it->second;
    }   
    const long double term = pow(2.0, -max);
    sum += term;
  }

  // Next, calculate the harmonic mean
  const double harmonic_mean = m * (1.0 / sum);

  // The harmonic mean is scaled by a constant that depends on the precision.
  const long double estimate = EMP_alpha(register_count_) * m *harmonic_mean;

  //cout << "Raw estimate thinks it's " << estimate << "\n";
  return estimate;
}

uint64_t SHLL::Estimate(uint32_t window, uint32_t timestamp) const {
  //cout << "Max window is " << max_window_ << "\n";
  // TODO(tdial): The logic below was more or less copied from the research
  // paper, less the handling of the sparse register array, which is not
  // implemented at this time. It is correct, but seems a little awkward.
  // Have someone else review this.

  // First, calculate the raw estimate per original HyperLogLog.
  const long double E = RawEstimate(window, timestamp);
  // Determine the threshold under which we apply a bias correction.
  const double BiasThreshold = 5 * register_count_;

  // Calculate E', the bias corrected estimate.
  // had problems with bias correction at low precision
  const double EP = (E < BiasThreshold && precision_ >= 6) ? (E - EMP_bias(E, precision_)) : E;


  // The number of zeroed registers decides whether we use LinearCounting.
  const uint16_t V = RegistersEqualToZero(window, timestamp);
  // H is either the LinearCounting estimate or the bias-corrected estimate.
  double H = 0.0;
  if (V != 0) {
    H = LinearCounting(register_count_, V);
  } else {
    H = EP;
  }

  // Under an empirically-determined threshold we return H, otherwise E'.
  if (H < EMP_threshold(precision_)) {
    return H;
  } else {
    return EP;
  }
}

int SHLL::get_size() {
  int sum = 0;
  for (int i = 0; i < register_count_; ++i) {
     sum += (*registers_.get())[i].size();
  }
  return sum;
}

uint16_t SHLL::RegistersEqualToZero(uint32_t window, uint32_t timestamp) const {
  uint16_t zeroed_registers = 0;
  for (int i = 0; i < register_count_; ++i) 
    if ((*registers_.get())[i].empty()) 
       ++zeroed_registers;
     else 
       if ((*registers_.get())[i].front().first + window < timestamp) ++zeroed_registers;
          
  return zeroed_registers;
}

/*uint64_t hash(uint64_t i) {
   uint64_t hash = 14695981039346656037ULL;
   for (int j = 0; j < 8; j++) {
      uint64_t octet = (i & (0xff << j))>>j;
      hash = hash ^ octet;
      hash = hash * 1099511628211;
   }
   //cout << "label_to_hash[" << i << "] = " << hash << "\n";
   return hash;
}*/

uint64_t hash(uint64_t i) {
  // Structure that is 160 bits wide used to extract 64 bits from a SHA-1.
  struct hashval {
    uint64_t high64;
    char low96[12];
  } hash;

  // Calculate the SHA-1 hash of the integer.
  SHA_CTX ctx;
  SHA1_Init(&ctx);
  i = i % (0xffffffffffffffff);
  SHA1_Update(&ctx, (unsigned char*)&i, sizeof(i));
  SHA1_Final((unsigned char*)&hash, &ctx);

  // Return 64 bits of the hash.
  return hash.high64;
}

}  // namespace libcount
