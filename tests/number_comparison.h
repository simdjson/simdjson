#ifndef NUMBER_COMPARISON_H
#define NUMBER_COMPARISON_H

#include <cinttypes>
#include <cstring>

namespace number_tests {

// ulp distance
// Marc B. Reynolds, 2016-2019
// Public Domain under http://unlicense.org, see link for details.
// adapted by D. Lemire
inline uint32_t f32_ulp_dist(float a, float b) {
  uint32_t ua, ub;
  memcpy(&ua, &a, sizeof(ua));
  memcpy(&ub, &b, sizeof(ub));
  if ((int32_t)(ub ^ ua) >= 0)
    return (int32_t)(ua - ub) >= 0 ? (ua - ub) : (ub - ua);
  return ua + ub + 0x80000000;
}

// ulp distance
// Marc B. Reynolds, 2016-2019
// Public Domain under http://unlicense.org, see link for details.
// adapted by D. Lemire
inline uint64_t f64_ulp_dist(double a, double b) {
  uint64_t ua, ub;
  std::memcpy(&ua, &a, sizeof(ua));
  std::memcpy(&ub, &b, sizeof(ub));
  if (static_cast<int64_t>(ub ^ ua) >= 0)
    return static_cast<int64_t>(ua - ub) >= 0 ? (ua - ub) : (ub - ua);
  return ua + ub + 0x80000000;
}
}
#endif // NUMBER_COMPARISON_H
