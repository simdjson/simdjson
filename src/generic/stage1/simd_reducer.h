#ifndef SIMDJSON_SRC_GENERIC_STAGE1_SIMD_REDUCER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_STAGE1_SIMD_REDUCER_H
#include <generic/stage1/base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {

/** Incrementally performs a reduce OR operation to accumulate an error. */
template <int N>
struct simd_or_reducer;

template<> struct simd_or_reducer<8> {
  simd8<uint8_t> error01234567;
  operator simd8<uint8_t>() && { return error01234567; }
};
template<> struct simd_or_reducer<7> {
  simd8<uint8_t> error0123;
  simd8<uint8_t> error45;
  simd8<uint8_t> error6;
  simd_or_reducer<8> next(simd8<uint8_t> error7) && { return { error0123 | error45 | error6 | error7 }; }
  operator simd8<uint8_t>() && { return error0123 | error45 | error6; }
};
template<> struct simd_or_reducer<6> {
  simd8<uint8_t>    error0123;
  simd8<uint8_t>    error45;
  // simd8<uint8_t> error_;
  simd_or_reducer<7> next(simd8<uint8_t> error6) && { return { error0123, error45, error6 }; }
  operator simd8<uint8_t>() && { return error0123 | error45; }
};
template<> struct simd_or_reducer<5> {
  simd8<uint8_t>    error0123;
  // simd8<uint8_t> error__;
  simd8<uint8_t>    error4;
  simd_or_reducer<6> next(simd8<uint8_t> error5) && { return { error0123, error4 | error5 }; }
  operator simd8<uint8_t>() && { return error0123 | error4; }
};
template<> struct simd_or_reducer<4> {
  simd8<uint8_t>    error0123;
  // simd8<uint8_t> error__;
  // simd8<uint8_t> error_;
  operator simd8<uint8_t>() && { return error0123; }
};
template<> struct simd_or_reducer<3> {
  // simd8<uint8_t> error____;
  simd8<uint8_t>    error01;
  simd8<uint8_t>    error2;
  simd_or_reducer<4> next(simd8<uint8_t> error3) && { return { error01 | error2 | error3 }; }
  operator simd8<uint8_t>() && { return error01 | error2; }
};
template<> struct simd_or_reducer<2> {
  // simd8<uint8_t> error____;
  simd8<uint8_t>    error01;
  // simd8<uint8_t> error_;
  simd_or_reducer<3> next(simd8<uint8_t> error2) && { return { error01, error2 }; }
  operator simd8<uint8_t>() && { return error01; }
};
template<> struct simd_or_reducer<1> {
  // simd8<uint8_t> error____;
  // simd8<uint8_t> error__;
  simd8<uint8_t>    error0;
  simd_or_reducer<2> next(simd8<uint8_t> error1) && { return { error0 | error1 }; }
  operator simd8<uint8_t>() && { return error0; }
};
template<> struct simd_or_reducer<0> {
  simd8<uint8_t> prev_error{};
  simd_or_reducer<1> next(simd8<uint8_t> error0) && { return { prev_error | error0 }; }
  operator simd8<uint8_t>() && { return prev_error; }
};

} // namespace simd
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_STAGE1_SIMD_REDUCER_H
