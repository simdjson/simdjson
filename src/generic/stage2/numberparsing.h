#include <cmath>
#include <limits>


namespace {
namespace SIMDJSON_IMPLEMENTATION {
namespace stage2 {
namespace numberparsing {



/**
 * The code in the slow_from_chars namespace is meant to handle the case where
 * we have more than 19 digits.
 *
 * It is based on work by Nigel Tao (at https://github.com/google/wuffs/)
 * who credits Ken Thompson for the design (via a reference to the Go source
 * code). See
 * https://github.com/google/wuffs/blob/aa46859ea40c72516deffa1b146121952d6dfd3b/internal/cgen/base/floatconv-submodule-data.c
 * https://github.com/google/wuffs/blob/46cd8105f47ca07ae2ba8e6a7818ef9c0df6c152/internal/cgen/base/floatconv-submodule-code.c
 * It is probably not very fast but it is a fallback that should almost never be
 * called in real life. Google Wuffs is published under APL 2.0.
 **/
namespace slow_from_chars {

namespace {
constexpr uint32_t max_digits = 768;
constexpr int32_t decimal_point_range = 2047;
} // namespace

struct adjusted_mantissa {
  uint64_t mantissa;
  int power2;
  adjusted_mantissa() : mantissa(0), power2(0) {}
};

struct decimal {
  uint32_t num_digits;
  int32_t decimal_point;
  bool negative;
  bool truncated;
  uint8_t digits[max_digits];
};

template <typename T> struct binary_format {
  static constexpr int mantissa_explicit_bits();
  static constexpr int minimum_exponent();
  static constexpr int infinite_power();
  static constexpr int sign_index();
};

template <> constexpr int binary_format<double>::mantissa_explicit_bits() {
  return 52;
}

template <> constexpr int binary_format<double>::minimum_exponent() {
  return -1023;
}
template <> constexpr int binary_format<double>::infinite_power() {
  return 0x7FF;
}

template <> constexpr int binary_format<double>::sign_index() { return 63; }

bool is_integer(char c)  noexcept  { return (c >= '0' && c <= '9'); }

// This should always succeed since it follows a call to parse_number.
decimal parse_decimal(const char *&p) noexcept {
  decimal answer;
  answer.num_digits = 0;
  answer.decimal_point = 0;
  answer.negative = false;
  answer.truncated = false;
  answer.negative = (*p == '-');
  if ((*p == '-') || (*p == '+')) {
    ++p;
  }

  while (*p == '0') {
    ++p;
  }
  while (is_integer(*p)) {
    if (answer.num_digits + 1 < max_digits) {
      answer.digits[answer.num_digits++] = uint8_t(*p - '0');
    } else {
      answer.truncated = true;
    }
    ++p;
  }
  const char *first_after_period{};
  if (*p == '.') {
    ++p;
    first_after_period = p;
    // if we have not yet encountered a zero, we have to skip it as well
    if (answer.num_digits == 0) {
      // skip zeros
      while (*p == '0') {
        ++p;
      }
    }
    while (is_integer(*p)) {
      if (answer.num_digits + 1 < max_digits) {
        answer.digits[answer.num_digits++] = uint8_t(*p - '0');
      } else {
        answer.truncated = true;
      }
      ++p;
    }
    answer.decimal_point = int32_t(first_after_period - p);
  }

  if (('e' == *p) || ('E' == *p)) {
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    int32_t exp_number = 0; // exponential part
    while (is_integer(*p)) {
      uint8_t digit = uint8_t(*p - '0');
      if (exp_number < 0x10000) {
        exp_number = 10 * exp_number + digit;
      }
      ++p;
    }
    answer.decimal_point += (neg_exp ? -exp_number : exp_number);
  }
  answer.decimal_point += answer.num_digits;
  return answer;
}

namespace {

// remove all final zeroes
inline void trim(decimal &h) {
  while ((h.num_digits > 0) && (h.digits[h.num_digits - 1] == 0)) {
    h.num_digits--;
  }
}

uint32_t number_of_digits_decimal_left_shift(decimal &h, uint32_t shift) {
  shift &= 63;
  const static uint16_t number_of_digits_decimal_left_shift_table[65] = {
      0x0000, 0x0800, 0x0801, 0x0803, 0x1006, 0x1009, 0x100D, 0x1812, 0x1817,
      0x181D, 0x2024, 0x202B, 0x2033, 0x203C, 0x2846, 0x2850, 0x285B, 0x3067,
      0x3073, 0x3080, 0x388E, 0x389C, 0x38AB, 0x38BB, 0x40CC, 0x40DD, 0x40EF,
      0x4902, 0x4915, 0x4929, 0x513E, 0x5153, 0x5169, 0x5180, 0x5998, 0x59B0,
      0x59C9, 0x61E3, 0x61FD, 0x6218, 0x6A34, 0x6A50, 0x6A6D, 0x6A8B, 0x72AA,
      0x72C9, 0x72E9, 0x7B0A, 0x7B2B, 0x7B4D, 0x8370, 0x8393, 0x83B7, 0x83DC,
      0x8C02, 0x8C28, 0x8C4F, 0x9477, 0x949F, 0x94C8, 0x9CF2, 0x051C, 0x051C,
      0x051C, 0x051C,
  };
  uint32_t x_a = number_of_digits_decimal_left_shift_table[shift];
  uint32_t x_b = number_of_digits_decimal_left_shift_table[shift + 1];
  uint32_t num_new_digits = x_a >> 11;
  uint32_t pow5_a = 0x7FF & x_a;
  uint32_t pow5_b = 0x7FF & x_b;
  const static uint8_t
      number_of_digits_decimal_left_shift_table_powers_of_5[0x051C] = {
          5, 2, 5, 1, 2, 5, 6, 2, 5, 3, 1, 2, 5, 1, 5, 6, 2, 5, 7, 8, 1, 2, 5,
          3, 9, 0, 6, 2, 5, 1, 9, 5, 3, 1, 2, 5, 9, 7, 6, 5, 6, 2, 5, 4, 8, 8,
          2, 8, 1, 2, 5, 2, 4, 4, 1, 4, 0, 6, 2, 5, 1, 2, 2, 0, 7, 0, 3, 1, 2,
          5, 6, 1, 0, 3, 5, 1, 5, 6, 2, 5, 3, 0, 5, 1, 7, 5, 7, 8, 1, 2, 5, 1,
          5, 2, 5, 8, 7, 8, 9, 0, 6, 2, 5, 7, 6, 2, 9, 3, 9, 4, 5, 3, 1, 2, 5,
          3, 8, 1, 4, 6, 9, 7, 2, 6, 5, 6, 2, 5, 1, 9, 0, 7, 3, 4, 8, 6, 3, 2,
          8, 1, 2, 5, 9, 5, 3, 6, 7, 4, 3, 1, 6, 4, 0, 6, 2, 5, 4, 7, 6, 8, 3,
          7, 1, 5, 8, 2, 0, 3, 1, 2, 5, 2, 3, 8, 4, 1, 8, 5, 7, 9, 1, 0, 1, 5,
          6, 2, 5, 1, 1, 9, 2, 0, 9, 2, 8, 9, 5, 5, 0, 7, 8, 1, 2, 5, 5, 9, 6,
          0, 4, 6, 4, 4, 7, 7, 5, 3, 9, 0, 6, 2, 5, 2, 9, 8, 0, 2, 3, 2, 2, 3,
          8, 7, 6, 9, 5, 3, 1, 2, 5, 1, 4, 9, 0, 1, 1, 6, 1, 1, 9, 3, 8, 4, 7,
          6, 5, 6, 2, 5, 7, 4, 5, 0, 5, 8, 0, 5, 9, 6, 9, 2, 3, 8, 2, 8, 1, 2,
          5, 3, 7, 2, 5, 2, 9, 0, 2, 9, 8, 4, 6, 1, 9, 1, 4, 0, 6, 2, 5, 1, 8,
          6, 2, 6, 4, 5, 1, 4, 9, 2, 3, 0, 9, 5, 7, 0, 3, 1, 2, 5, 9, 3, 1, 3,
          2, 2, 5, 7, 4, 6, 1, 5, 4, 7, 8, 5, 1, 5, 6, 2, 5, 4, 6, 5, 6, 6, 1,
          2, 8, 7, 3, 0, 7, 7, 3, 9, 2, 5, 7, 8, 1, 2, 5, 2, 3, 2, 8, 3, 0, 6,
          4, 3, 6, 5, 3, 8, 6, 9, 6, 2, 8, 9, 0, 6, 2, 5, 1, 1, 6, 4, 1, 5, 3,
          2, 1, 8, 2, 6, 9, 3, 4, 8, 1, 4, 4, 5, 3, 1, 2, 5, 5, 8, 2, 0, 7, 6,
          6, 0, 9, 1, 3, 4, 6, 7, 4, 0, 7, 2, 2, 6, 5, 6, 2, 5, 2, 9, 1, 0, 3,
          8, 3, 0, 4, 5, 6, 7, 3, 3, 7, 0, 3, 6, 1, 3, 2, 8, 1, 2, 5, 1, 4, 5,
          5, 1, 9, 1, 5, 2, 2, 8, 3, 6, 6, 8, 5, 1, 8, 0, 6, 6, 4, 0, 6, 2, 5,
          7, 2, 7, 5, 9, 5, 7, 6, 1, 4, 1, 8, 3, 4, 2, 5, 9, 0, 3, 3, 2, 0, 3,
          1, 2, 5, 3, 6, 3, 7, 9, 7, 8, 8, 0, 7, 0, 9, 1, 7, 1, 2, 9, 5, 1, 6,
          6, 0, 1, 5, 6, 2, 5, 1, 8, 1, 8, 9, 8, 9, 4, 0, 3, 5, 4, 5, 8, 5, 6,
          4, 7, 5, 8, 3, 0, 0, 7, 8, 1, 2, 5, 9, 0, 9, 4, 9, 4, 7, 0, 1, 7, 7,
          2, 9, 2, 8, 2, 3, 7, 9, 1, 5, 0, 3, 9, 0, 6, 2, 5, 4, 5, 4, 7, 4, 7,
          3, 5, 0, 8, 8, 6, 4, 6, 4, 1, 1, 8, 9, 5, 7, 5, 1, 9, 5, 3, 1, 2, 5,
          2, 2, 7, 3, 7, 3, 6, 7, 5, 4, 4, 3, 2, 3, 2, 0, 5, 9, 4, 7, 8, 7, 5,
          9, 7, 6, 5, 6, 2, 5, 1, 1, 3, 6, 8, 6, 8, 3, 7, 7, 2, 1, 6, 1, 6, 0,
          2, 9, 7, 3, 9, 3, 7, 9, 8, 8, 2, 8, 1, 2, 5, 5, 6, 8, 4, 3, 4, 1, 8,
          8, 6, 0, 8, 0, 8, 0, 1, 4, 8, 6, 9, 6, 8, 9, 9, 4, 1, 4, 0, 6, 2, 5,
          2, 8, 4, 2, 1, 7, 0, 9, 4, 3, 0, 4, 0, 4, 0, 0, 7, 4, 3, 4, 8, 4, 4,
          9, 7, 0, 7, 0, 3, 1, 2, 5, 1, 4, 2, 1, 0, 8, 5, 4, 7, 1, 5, 2, 0, 2,
          0, 0, 3, 7, 1, 7, 4, 2, 2, 4, 8, 5, 3, 5, 1, 5, 6, 2, 5, 7, 1, 0, 5,
          4, 2, 7, 3, 5, 7, 6, 0, 1, 0, 0, 1, 8, 5, 8, 7, 1, 1, 2, 4, 2, 6, 7,
          5, 7, 8, 1, 2, 5, 3, 5, 5, 2, 7, 1, 3, 6, 7, 8, 8, 0, 0, 5, 0, 0, 9,
          2, 9, 3, 5, 5, 6, 2, 1, 3, 3, 7, 8, 9, 0, 6, 2, 5, 1, 7, 7, 6, 3, 5,
          6, 8, 3, 9, 4, 0, 0, 2, 5, 0, 4, 6, 4, 6, 7, 7, 8, 1, 0, 6, 6, 8, 9,
          4, 5, 3, 1, 2, 5, 8, 8, 8, 1, 7, 8, 4, 1, 9, 7, 0, 0, 1, 2, 5, 2, 3,
          2, 3, 3, 8, 9, 0, 5, 3, 3, 4, 4, 7, 2, 6, 5, 6, 2, 5, 4, 4, 4, 0, 8,
          9, 2, 0, 9, 8, 5, 0, 0, 6, 2, 6, 1, 6, 1, 6, 9, 4, 5, 2, 6, 6, 7, 2,
          3, 6, 3, 2, 8, 1, 2, 5, 2, 2, 2, 0, 4, 4, 6, 0, 4, 9, 2, 5, 0, 3, 1,
          3, 0, 8, 0, 8, 4, 7, 2, 6, 3, 3, 3, 6, 1, 8, 1, 6, 4, 0, 6, 2, 5, 1,
          1, 1, 0, 2, 2, 3, 0, 2, 4, 6, 2, 5, 1, 5, 6, 5, 4, 0, 4, 2, 3, 6, 3,
          1, 6, 6, 8, 0, 9, 0, 8, 2, 0, 3, 1, 2, 5, 5, 5, 5, 1, 1, 1, 5, 1, 2,
          3, 1, 2, 5, 7, 8, 2, 7, 0, 2, 1, 1, 8, 1, 5, 8, 3, 4, 0, 4, 5, 4, 1,
          0, 1, 5, 6, 2, 5, 2, 7, 7, 5, 5, 5, 7, 5, 6, 1, 5, 6, 2, 8, 9, 1, 3,
          5, 1, 0, 5, 9, 0, 7, 9, 1, 7, 0, 2, 2, 7, 0, 5, 0, 7, 8, 1, 2, 5, 1,
          3, 8, 7, 7, 7, 8, 7, 8, 0, 7, 8, 1, 4, 4, 5, 6, 7, 5, 5, 2, 9, 5, 3,
          9, 5, 8, 5, 1, 1, 3, 5, 2, 5, 3, 9, 0, 6, 2, 5, 6, 9, 3, 8, 8, 9, 3,
          9, 0, 3, 9, 0, 7, 2, 2, 8, 3, 7, 7, 6, 4, 7, 6, 9, 7, 9, 2, 5, 5, 6,
          7, 6, 2, 6, 9, 5, 3, 1, 2, 5, 3, 4, 6, 9, 4, 4, 6, 9, 5, 1, 9, 5, 3,
          6, 1, 4, 1, 8, 8, 8, 2, 3, 8, 4, 8, 9, 6, 2, 7, 8, 3, 8, 1, 3, 4, 7,
          6, 5, 6, 2, 5, 1, 7, 3, 4, 7, 2, 3, 4, 7, 5, 9, 7, 6, 8, 0, 7, 0, 9,
          4, 4, 1, 1, 9, 2, 4, 4, 8, 1, 3, 9, 1, 9, 0, 6, 7, 3, 8, 2, 8, 1, 2,
          5, 8, 6, 7, 3, 6, 1, 7, 3, 7, 9, 8, 8, 4, 0, 3, 5, 4, 7, 2, 0, 5, 9,
          6, 2, 2, 4, 0, 6, 9, 5, 9, 5, 3, 3, 6, 9, 1, 4, 0, 6, 2, 5,
      };
  const uint8_t *pow5 =
      &number_of_digits_decimal_left_shift_table_powers_of_5[pow5_a];
  uint32_t i = 0;
  uint32_t n = pow5_b - pow5_a;
  for (; i < n; i++) {
    if (i >= h.num_digits) {
      return num_new_digits - 1;
    } else if (h.digits[i] == pow5[i]) {
      continue;
    } else if (h.digits[i] < pow5[i]) {
      return num_new_digits - 1;
    } else {
      return num_new_digits;
    }
  }
  return num_new_digits;
}

} // end of anonymous namespace

uint64_t round(decimal &h) {
  if ((h.num_digits == 0) || (h.decimal_point < 0)) {
    return 0;
  } else if (h.decimal_point > 18) {
    return UINT64_MAX;
  }
  // at this point, we know that h.decimal_point >= 0
  uint32_t dp = uint32_t(h.decimal_point);
  uint64_t n = 0;
  for (uint32_t i = 0; i < dp; i++) {
    n = (10 * n) + ((i < h.num_digits) ? h.digits[i] : 0);
  }
  bool round_up = false;
  if (dp < h.num_digits) {
    round_up = h.digits[dp] >= 5; // normally, we round up
    // but we may need to round to even!
    if ((h.digits[dp] == 5) && (dp + 1 == h.num_digits)) {
      round_up = h.truncated || ((dp > 0) && (1 & h.digits[dp - 1]));
    }
  }
  if (round_up) {
    n++;
  }
  return n;
}

// computes h * 2^-shift
void decimal_left_shift(decimal &h, uint32_t shift) {
  if (h.num_digits == 0) {
    return;
  }
  uint32_t num_new_digits = number_of_digits_decimal_left_shift(h, shift);
  int32_t read_index = int32_t(h.num_digits - 1);
  uint32_t write_index = h.num_digits - 1 + num_new_digits;
  uint64_t n = 0;

  while (read_index >= 0) {
    n += uint64_t(h.digits[read_index]) << shift;
    uint64_t quotient = n / 10;
    uint64_t remainder = n - (10 * quotient);
    if (write_index < max_digits) {
      h.digits[write_index] = uint8_t(remainder);
    } else if (remainder > 0) {
      h.truncated = true;
    }
    n = quotient;
    write_index--;
    read_index--;
  }
  while (n > 0) {
    uint64_t quotient = n / 10;
    uint64_t remainder = n - (10 * quotient);
    if (write_index < max_digits) {
      h.digits[write_index] = uint8_t(remainder);
    } else if (remainder > 0) {
      h.truncated = true;
    }
    n = quotient;
    write_index--;
  }
  h.num_digits += num_new_digits;
  if (h.num_digits > max_digits) {
    h.num_digits = max_digits;
  }
  h.decimal_point += int32_t(num_new_digits);
  trim(h);
}

// computes h * 2^shift
void decimal_right_shift(decimal &h, uint32_t shift) {
  uint32_t read_index = 0;
  uint32_t write_index = 0;

  uint64_t n = 0;

  while ((n >> shift) == 0) {
    if (read_index < h.num_digits) {
      n = (10 * n) + h.digits[read_index++];
    } else if (n == 0) {
      return;
    } else {
      while ((n >> shift) == 0) {
        n = 10 * n;
        read_index++;
      }
      break;
    }
  }
  h.decimal_point -= int32_t(read_index - 1);
  if (h.decimal_point < -decimal_point_range) { // it is zero
    h.num_digits = 0;
    h.decimal_point = 0;
    h.negative = false;
    h.truncated = false;
    return;
  }
  uint64_t mask = (uint64_t(1) << shift) - 1;
  while (read_index < h.num_digits) {
    uint8_t new_digit = uint8_t(n >> shift);
    n = (10 * (n & mask)) + h.digits[read_index++];
    h.digits[write_index++] = new_digit;
  }
  while (n > 0) {
    uint8_t new_digit = uint8_t(n >> shift);
    n = 10 * (n & mask);
    if (write_index < max_digits) {
      h.digits[write_index++] = new_digit;
    } else if (new_digit > 0) {
      h.truncated = true;
    }
  }
  h.num_digits = write_index;
  trim(h);
}

template <typename binary> adjusted_mantissa compute_float(decimal &d) {
  adjusted_mantissa answer;
  if (d.num_digits == 0) {
    // should be zero
    answer.power2 = 0;
    answer.mantissa = 0;
    return answer;
  }
  // At this point, going further, we can assume that d.num_digits > 0.
  // We want to guard against excessive decimal point values because
  // they can result in long running times. Indeed, we do 
  // shifts by at most 60 bits. We have that log(10**400)/log(2**60) ~= 22
  // which is fine, but log(10**299995)/log(2**60) ~= 16609 which is not
  // fine (runs for a long time).
  //
  if(d.decimal_point < -324) {
    // We have something smaller than 1e-324 which is always zero
    // in binary64 and binary32. 
    // It should be zero.
    answer.power2 = 0;
    answer.mantissa = 0;
    return answer;
  } else if(d.decimal_point >= 310) {
    // We have something at least as large as 0.1e310 which is
    // always infinite.    
    answer.power2 = binary::infinite_power();
    answer.mantissa = 0;
    return answer;
  }

  static const uint32_t max_shift = 60;
  static const uint32_t num_powers = 19;
  static const uint8_t powers[19] = {
      0,  3,  6,  9,  13, 16, 19, 23, 26, 29, //
      33, 36, 39, 43, 46, 49, 53, 56, 59,     //
  };
  int32_t exp2 = 0;
  while (d.decimal_point > 0) {
    uint32_t n = uint32_t(d.decimal_point);
    uint32_t shift = (n < num_powers) ? powers[n] : max_shift;
    decimal_right_shift(d, shift);
    if (d.decimal_point < -decimal_point_range) {
      // should be zero
      answer.power2 = 0;
      answer.mantissa = 0;
      return answer;
    }
    exp2 += int32_t(shift);
  }
  // We shift left toward [1/2 ... 1].
  while (d.decimal_point <= 0) {
    uint32_t shift;
    if (d.decimal_point == 0) {
      if (d.digits[0] >= 5) {
        break;
      }
      shift = (d.digits[0] < 2) ? 2 : 1;
    } else {
      uint32_t n = uint32_t(-d.decimal_point);
      shift = (n < num_powers) ? powers[n] : max_shift;
    }
    decimal_left_shift(d, shift);
    if (d.decimal_point > decimal_point_range) {
      // we want to get infinity:
      answer.power2 = 0xFF;
      answer.mantissa = 0;
      return answer;
    }
    exp2 -= int32_t(shift);
  }
  // We are now in the range [1/2 ... 1] but the binary format uses [1 ... 2].
  exp2--;
  constexpr int32_t minimum_exponent = binary::minimum_exponent();
  while ((minimum_exponent + 1) > exp2) {
    uint32_t n = uint32_t((minimum_exponent + 1) - exp2);
    if (n > max_shift) {
      n = max_shift;
    }
    decimal_right_shift(d, n);
    exp2 += int32_t(n);
  }
  if ((exp2 - minimum_exponent) >= binary::infinite_power()) {
    answer.power2 = binary::infinite_power();
    answer.mantissa = 0;
    return answer;
  }

  const int mantissa_size_in_bits = binary::mantissa_explicit_bits() + 1;
  decimal_left_shift(d, mantissa_size_in_bits);

  uint64_t mantissa = round(d);
  // It is possible that we have an overflow, in which case we need
  // to shift back.
  if (mantissa >= (uint64_t(1) << mantissa_size_in_bits)) {
    decimal_right_shift(d, 1);
    exp2 += 1;
    mantissa = round(d);
    if ((exp2 - minimum_exponent) >= binary::infinite_power()) {
      answer.power2 = binary::infinite_power();
      answer.mantissa = 0;
      return answer;
    }
  }
  answer.power2 = exp2 - binary::minimum_exponent();
  if (mantissa < (uint64_t(1) << binary::mantissa_explicit_bits())) {
    answer.power2--;
  }
  answer.mantissa =
      mantissa & ((uint64_t(1) << binary::mantissa_explicit_bits()) - 1);
  return answer;
}

template <typename binary>
adjusted_mantissa parse_long_mantissa(const char *first) {
  decimal d = parse_decimal(first);
  return compute_float<binary>(d);
}

double from_chars(const char *first) noexcept {
  bool negative = first[0] == '-';
  if (negative) {
    first++;
  }
  adjusted_mantissa am = parse_long_mantissa<binary_format<double>>(first);
  uint64_t word = am.mantissa;
  word |= uint64_t(am.power2)
          << binary_format<double>::mantissa_explicit_bits();
  word = negative ? word | (uint64_t(1) << binary_format<double>::sign_index())
                  : word;
  double value;
  std::memcpy(&value, &word, sizeof(double));
  return value;
}

} // namespace slow_from_chars
/**
 * This ends the "fall back routine". What follows is the code that will actually be used in 
 * practice.
 */


#ifdef JSON_TEST_NUMBERS
#define INVALID_NUMBER(SRC) (found_invalid_number((SRC)), NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (found_integer((VALUE), (SRC)), (WRITER).append_s64((VALUE)))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (found_unsigned_integer((VALUE), (SRC)), (WRITER).append_u64((VALUE)))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (found_float((VALUE), (SRC)), (WRITER).append_double((VALUE)))
#else
#define INVALID_NUMBER(SRC) (NUMBER_ERROR)
#define WRITE_INTEGER(VALUE, SRC, WRITER) (WRITER).append_s64((VALUE))
#define WRITE_UNSIGNED(VALUE, SRC, WRITER) (WRITER).append_u64((VALUE))
#define WRITE_DOUBLE(VALUE, SRC, WRITER) (WRITER).append_double((VALUE))
#endif

// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [FASTFLOAT_SMALLEST_POWER,
// FASTFLOAT_LARGEST_POWER] interval: the caller is responsible for this check.
simdjson_really_inline bool compute_float_64(int64_t power, uint64_t i, bool negative, double &d) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
#ifndef FLT_EVAL_METHOD
#error "FLT_EVAL_METHOD should be defined, please include cfloat."
#endif
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // We cannot be certain that x/y is rounded to nearest.
  if (0 <= power && power <= 22 && i <= 9007199254740991) {
#else
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
#endif
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    d = double(i);
    //
    // The general idea is as follows.
    // If 0 <= s < 2^53 and if 10^0 <= p <= 10^22 then
    // 1) Both s and p can be represented exactly as 64-bit floating-point
    // values
    // (binary64).
    // 2) Because s and p can be represented exactly as floating-point values,
    // then s * p
    // and s / p will produce correctly rounded values.
    //
    if (power < 0) {
      d = d / power_of_ten[-power];
    } else {
      d = d * power_of_ten[power];
    }
    if (negative) {
      d = -d;
    }
    return true;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It was
  // described by David M. Gay in  "Correctly rounded
  // binary-decimal and decimal-binary conversions." (1990)
  // If you need to compute i * 10^(22 + x) for x < 16,
  // first compute i * 10^x, if you know that result is exact
  // (e.g., when i * 10^x < 2^53),
  // then you can still proceed and do (i * 10^x) * 10^22.
  // Is this worth your time?
  // You need  22 < power *and* power <  22 + 16 *and* (i * 10^(x-22) < 2^53)
  // for this second fast path to work.
  // If you you have 22 < power *and* power <  22 + 16, and then you
  // optimistically compute "i * 10^(x-22)", there is still a chance that you
  // have wasted your time if i * 10^(x-22) >= 2^53. It makes the use cases of
  // this optimization maybe less common than we would like. Source:
  // http://www.exploringbinary.com/fast-path-decimal-to-floating-point-conversion/
  // also used in RapidJSON: https://rapidjson.org/strtod_8h_source.html

  // The fast path has now failed, so we are failing back on the slower path.

  // In the slow path, we need to adjust i so that it is > 1<<63 which is always
  // possible, except if i == 0, so we handle i == 0 separately.
  if(i == 0) {
    d = 0.0;
    return true;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise product.
  // We use a table lookup approach.
  // It is safe because
  // power >= FASTFLOAT_SMALLEST_POWER
  // and power <= FASTFLOAT_LARGEST_POWER
  // We recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = mantissa_64[power - FASTFLOAT_SMALLEST_POWER];
  
  // The exponent is 1024 + 63 + power 
  //     + floor(log(5**power)/log(2)).
  // The 1024 comes from the ieee64 standard.
  // The 63 comes from the fact that we use a 64-bit word.
  //
  // Computing floor(log(5**power)/log(2)) could be
  // slow. Instead we use a fast function.
  //
  // For power in (-400,350), we have that
  // (((152170 + 65536) * power ) >> 16);
  // is equal to
  //  floor(log(5**power)/log(2)) + power
  //
  // The 65536 is (1<<16) and corresponds to 
  // (65536 * power) >> 16 ---> power
  //
  // ((152170 * power ) >> 16) is equal to 
  // floor(log(5**power)/log(2)) 
  //
  // Note that this is not magic: 152170/(1<<16) is 
  // approximatively equal to log(5)/log(2).
  // The 1<<16 value is a power of two; we could use a 
  // larger power of 2 if we wanted to.
  //
  int64_t exponent = (((152170 + 65536) * power) >> 16) + 1024 + 63;
  

  // We want the most significant bit of i to be 1. Shift if needed.
  int lz = leading_zeroes(i);
  i <<= lz;
  // We want the most significant 64 bits of the product. We know
  // this will be non-zero because the most significant bit of i is
  // 1.
  value128 product = full_multiplication(i, factor_mantissa);
  uint64_t lower = product.low;
  uint64_t upper = product.high;

  // We know that upper has at most one leading zero because
  // both i and  factor_mantissa have a leading one. This means
  // that the result is at least as large as ((1<<63)*(1<<63))/(1<<64).

  // As long as the first 9 bits of "upper" are not "1", then we
  // know that we have an exact computed value for the leading
  // 55 bits because any imprecision would play out as a +1, in
  // the worst case.
  if (simdjson_unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
    uint64_t factor_mantissa_low =
        mantissa_128[power - FASTFLOAT_SMALLEST_POWER];
    // next, we compute the 64-bit x 128-bit multiplication, getting a 192-bit
    // result (three 64-bit values)
    product = full_multiplication(i, factor_mantissa_low);
    uint64_t product_low = product.low;
    uint64_t product_middle2 = product.high;
    uint64_t product_middle1 = lower;
    uint64_t product_high = upper;
    uint64_t product_middle = product_middle1 + product_middle2;
    if (product_middle < product_middle1) {
      product_high++; // overflow carry
    }
    // We want to check whether mantissa *i + i would affect our result.
    // This does happen, e.g. with 7.3177701707893310e+15.
    if (((product_middle + 1 == 0) && ((product_high & 0x1FF) == 0x1FF) &&
         (product_low + i < product_low))) { // let us be prudent and bail out.
      return false;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += int(1 ^ upperbit);

  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (simdjson_unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
               ((mantissa & 3) == 1))) {
    // if mantissa & 1 == 1 we might need to round up.
    //
    // Scenarios:
    // 1. We are not in the middle. Then we should round up.
    //
    // 2. We are right in the middle. Whether we round up depends
    // on the last significant bit: if it is "one" then we round
    // up (round to even) otherwise, we do not.
    //
    // So if the last significant bit is 1, we can safely round up.
    // Hence we only need to bail out if (mantissa & 3) == 1.
    // Otherwise we may need more accuracy or analysis to determine whether
    // we are exactly between two floating-point numbers.
    // It can be triggered with 1e23.
    // Note: because the factor_mantissa and factor_mantissa_low are
    // almost always rounded down (except for small positive powers),
    // almost always should round up.
    return false;
  }

  mantissa += mantissa & 1;
  mantissa >>= 1;

  // Here we have mantissa < (1<<53), unless there was an overflow
  if (mantissa >= (1ULL << 53)) {
    //////////
    // This will happen when parsing values such as 7.2057594037927933e+16
    ////////
    mantissa = (1ULL << 52);
    lz--; // undo previous addition
  }
  mantissa &= ~(1ULL << 52);
  uint64_t real_exponent = exponent - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (simdjson_unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    return false;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  memcpy(&d, &mantissa, sizeof(d));
  return true;
}

static bool parse_float_strtod(const uint8_t *ptr, double *outDouble) {
  *outDouble = slow_from_chars::from_chars((const char *)ptr);
  if (!std::isfinite(*outDouble)) {
    return false;
  }
  return true;
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
simdjson_really_inline bool is_made_of_eight_digits_fast(const uint8_t *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING, "SIMDJSON_PADDING must be bigger than 7");
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

template<typename W>
error_code slow_float_parsing(SIMDJSON_UNUSED const uint8_t * src, W writer) {
  double d;
  if (parse_float_strtod(src, &d)) {
    writer.append_double(d);
    return SUCCESS;
  }
  return INVALID_NUMBER(src);
}

template<typename I>
NO_SANITIZE_UNDEFINED // We deliberately allow overflow here and check later
simdjson_really_inline bool parse_digit(const uint8_t c, I &i) {
  const uint8_t digit = static_cast<uint8_t>(c - '0');
  if (digit > 9) {
    return false;
  }
  // PERF NOTE: multiplication by 10 is cheaper than arbitrary integer multiplication
  i = 10 * i + digit; // might overflow, we will handle the overflow later
  return true;
}

simdjson_really_inline error_code parse_decimal(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, uint64_t &i, int64_t &exponent) {
  // we continue with the fiction that we have an integer. If the
  // floating point number is representable as x * 10^z for some integer
  // z that fits in 53 bits, then we will be able to convert back the
  // the integer into a float in a lossless manner.
  const uint8_t *const first_after_period = p;

#ifdef SWAR_NUMBER_PARSING
  // this helps if we have lots of decimals!
  // this turns out to be frequent enough.
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
  }
#endif
  // Unrolling the first digit makes a small difference on some implementations (e.g. westmere)
  if (parse_digit(*p, i)) { ++p; }
  while (parse_digit(*p, i)) { p++; }
  exponent = first_after_period - p;
  // Decimal without digits (123.) is illegal
  if (exponent == 0) {
    return INVALID_NUMBER(src);
  }
  return SUCCESS;
}

simdjson_really_inline error_code parse_exponent(SIMDJSON_UNUSED const uint8_t *const src, const uint8_t *&p, int64_t &exponent) {
  // Exp Sign: -123.456e[-]78
  bool neg_exp = ('-' == *p);
  if (neg_exp || '+' == *p) { p++; } // Skip + as well

  // Exponent: -123.456e-[78]
  auto start_exp = p;
  int64_t exp_number = 0;
  while (parse_digit(*p, exp_number)) { ++p; }
  // It is possible for parse_digit to overflow. 
  // In particular, it could overflow to INT64_MIN, and we cannot do - INT64_MIN.
  // Thus we *must* check for possible overflow before we negate exp_number.

  // Performance notes: it may seem like combining the two "simdjson_unlikely checks" below into
  // a single simdjson_unlikely path would be faster. The reasoning is sound, but the compiler may
  // not oblige and may, in fact, generate two distinct paths in any case. It might be
  // possible to do uint64_t(p - start_exp - 1) >= 18 but it could end up trading off 
  // instructions for a simdjson_likely branch, an unconclusive gain.

  // If there were no digits, it's an error.
  if (simdjson_unlikely(p == start_exp)) {
    return INVALID_NUMBER(src);
  }
  // We have a valid positive exponent in exp_number at this point, except that
  // it may have overflowed.

  // If there were more than 18 digits, we may have overflowed the integer. We have to do 
  // something!!!!
  if (simdjson_unlikely(p > start_exp+18)) {
    // Skip leading zeroes: 1e000000000000000000001 is technically valid and doesn't overflow
    while (*start_exp == '0') { start_exp++; }
    // 19 digits could overflow int64_t and is kind of absurd anyway. We don't
    // support exponents smaller than -999,999,999,999,999,999 and bigger
    // than 999,999,999,999,999,999.
    // We can truncate.
    // Note that 999999999999999999 is assuredly too large. The maximal ieee64 value before
    // infinity is ~1.8e308. The smallest subnormal is ~5e-324. So, actually, we could
    // truncate at 324.
    // Note that there is no reason to fail per se at this point in time. 
    // E.g., 0e999999999999999999999 is a fine number.
    if (p > start_exp+18) { exp_number = 999999999999999999; }
  }
  // At this point, we know that exp_number is a sane, positive, signed integer.
  // It is <= 999,999,999,999,999,999. As long as 'exponent' is in 
  // [-8223372036854775808, 8223372036854775808], we won't overflow. Because 'exponent'
  // is bounded in magnitude by the size of the JSON input, we are fine in this universe.
  // To sum it up: the next line should never overflow.
  exponent += (neg_exp ? -exp_number : exp_number);
  return SUCCESS;
}

simdjson_really_inline int significant_digits(const uint8_t * start_digits, int digit_count) {
  // It is possible that the integer had an overflow.
  // We have to handle the case where we have 0.0000somenumber.
  const uint8_t *start = start_digits;
  while ((*start == '0') || (*start == '.')) {
    start++;
  }
  // we over-decrement by one when there is a '.'
  return digit_count - int(start - start_digits);
}

template<typename W>
simdjson_really_inline error_code write_float(const uint8_t *const src, bool negative, uint64_t i, const uint8_t * start_digits, int digit_count, int64_t exponent, W &writer) {
  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon in practice.
  // digit count is off by 1 because of the decimal (assuming there was one).
  //
  // 9999999999999999999 < 2**64 so we can accomodate 19 digits.
  // If we have a decimal separator, then digit_count - 1 is the number of digits, but we
  // may not have a decimal separator!
  if (simdjson_unlikely(digit_count > 19 && significant_digits(start_digits, digit_count) > 19)) {
    // Ok, chances are good that we had an overflow!
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // This will happen in the following examples:
    // 10000000000000000000000000000000000000000000e+308
    // 3.1415926535897932384626433832795028841971693993751
    //
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  // NOTE: it's weird that the simdjson_unlikely() only wraps half the if, but it seems to get slower any other
  // way we've tried: https://github.com/simdjson/simdjson/pull/990#discussion_r448497331
  // To future reader: we'd love if someone found a better way, or at least could explain this result!
  if (simdjson_unlikely(exponent < FASTFLOAT_SMALLEST_POWER) || (exponent > FASTFLOAT_LARGEST_POWER)) {
    // this is almost never going to get called!!!
    // we start anew, going slowly!!!
    // NOTE: This makes a *copy* of the writer and passes it to slow_float_parsing. This happens
    // because slow_float_parsing is a non-inlined function. If we passed our writer reference to
    // it, it would force it to be stored in memory, preventing the compiler from picking it apart
    // and putting into registers. i.e. if we pass it as reference, it gets slow.
    // This is what forces the skip_double, as well.
    error_code error = slow_float_parsing(src, writer);
    writer.skip_double();
    return error;
  }
  double d;
  if (!compute_float_64(exponent, i, negative, d)) {
    // we are almost never going to get here.
    if (!parse_float_strtod(src, &d)) { return INVALID_NUMBER(src); }
  }
  WRITE_DOUBLE(d, src, writer);
  return SUCCESS;
}

// for performance analysis, it is sometimes  useful to skip parsing
#ifdef SIMDJSON_SKIPNUMBERPARSING

template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const, W &writer) {
  writer.append_s64(0);        // always write zero
  return SUCCESS;              // always succeeds
}

#else

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
template<typename W>
simdjson_really_inline error_code parse_number(const uint8_t *const src, W &writer) {

  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return INVALID_NUMBER(src); }

  //
  // Handle floats if there is a . or e (or both)
  //
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_decimal(src, p, i, exponent) );
    digit_count = int(p - start_digits); // used later to guard against overflows
  }
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    SIMDJSON_TRY( parse_exponent(src, p, exponent) );
  }
  if (is_float) {
    const bool clean_end = is_structural_or_whitespace(*p);
    SIMDJSON_TRY( write_float(src, negative, i, start_digits, digit_count, exponent, writer) );
    if (!clean_end) { return INVALID_NUMBER(src); }
    return SUCCESS;
  }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return INVALID_NUMBER(src); }
  if (digit_count == longest_digit_count) {
    if (negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return INVALID_NUMBER(src);  }
      WRITE_INTEGER(~i+1, src, writer);
      if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
      return SUCCESS;
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    }  else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return INVALID_NUMBER(src); }
  }

  // Write unsigned if it doesn't fit in a signed integer.
  if (i > uint64_t(INT64_MAX)) {
    WRITE_UNSIGNED(i, src, writer);
  } else {
    WRITE_INTEGER(negative ? (~i+1) : i, src, writer);
  }
  if (!is_structural_or_whitespace(*p)) { return INVALID_NUMBER(src); }
  return SUCCESS;
}

// SAX functions
namespace {
// Parse any number from 0 to 18,446,744,073,709,551,615
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  if (digit_count > 20) { return NUMBER_ERROR; }
  if (digit_count == 20) {
    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return i;
}

// Parse any number from 0 to 18,446,744,073,709,551,615
// Call this version of the method if you regularly expect 8- or 16-digit numbers.
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<uint64_t> parse_large_unsigned(const uint8_t * const src) noexcept {
  const uint8_t *p = src;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  if (is_made_of_eight_digits_fast(p)) {
    i = i * 100000000 + parse_eight_digits_unrolled(p);
    p += 8;
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
      if (parse_digit(*p, i)) { // digit 17
        p++;
        if (parse_digit(*p, i)) { // digit 18
          p++;
          if (parse_digit(*p, i)) { // digit 19
            p++;
            if (parse_digit(*p, i)) { // digit 20
              p++;
              if (parse_digit(*p, i)) { return NUMBER_ERROR; } // 21 digits is an error
              // Positive overflow check:
              // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
              //   biggest uint64_t.
              // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
              //   If we got here, it's a 20 digit number starting with the digit "1".
              // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
              //   than 1,553,255,926,290,448,384.
              // - That is smaller than the smallest possible 20-digit number the user could write:
              //   10,000,000,000,000,000,000.
              // - Therefore, if the number is positive and lower than that, it's overflow.
              // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
              //
              if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
            }
          }
        }
      }
    } // 16 digits
  } else { // 8 digits
    // Less than 8 digits can't overflow, simpler logic here.
    if (parse_digit(*p, i)) { p++; } else { return NUMBER_ERROR; }
    while (parse_digit(*p, i)) { p++; }
  }

  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }
  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - src);
  if (digit_count == 0 || ('0' == *src && digit_count > 1)) { return NUMBER_ERROR; }
  return i;
}

// Parse any number from  -9,223,372,036,854,775,808 to 9,223,372,036,854,775,807
SIMDJSON_UNUSED simdjson_really_inline simdjson_result<int64_t> parse_integer(const uint8_t *src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  const uint8_t *p = src + negative;

  //
  // Parse the integer part.
  //
  // PERF NOTE: we don't use is_made_of_eight_digits_fast because large integers like 123456789 are rare
  const uint8_t *const start_digits = p;
  uint64_t i = 0;
  while (parse_digit(*p, i)) { p++; }

  // If there were no digits, or if the integer starts with 0 and has more than one digit, it's an error.
  int digit_count = int(p - start_digits);
  if (digit_count == 0 || ('0' == *start_digits && digit_count > 1)) { return NUMBER_ERROR; }
  if (!is_structural_or_whitespace(*p)) { return NUMBER_ERROR; }

  // The longest negative 64-bit number is 19 digits.
  // The longest positive 64-bit number is 20 digits.
  // We do it this way so we don't trigger this branch unless we must.
  int longest_digit_count = negative ? 19 : 20;
  if (digit_count > longest_digit_count) { return NUMBER_ERROR; }
  if (digit_count == longest_digit_count) {
    if(negative) {
      // Anything negative above INT64_MAX+1 is invalid
      if (i > uint64_t(INT64_MAX)+1) { return NUMBER_ERROR; }
      return ~i+1;

    // Positive overflow check:
    // - A 20 digit number starting with 2-9 is overflow, because 18,446,744,073,709,551,615 is the
    //   biggest uint64_t.
    // - A 20 digit number starting with 1 is overflow if it is less than INT64_MAX.
    //   If we got here, it's a 20 digit number starting with the digit "1".
    // - If a 20 digit number starting with 1 overflowed (i*10+digit), the result will be smaller
    //   than 1,553,255,926,290,448,384.
    // - That is smaller than the smallest possible 20-digit number the user could write:
    //   10,000,000,000,000,000,000.
    // - Therefore, if the number is positive and lower than that, it's overflow.
    // - The value we are looking at is less than or equal to 9,223,372,036,854,775,808 (INT64_MAX).
    //
    } else if (src[0] != uint8_t('1') || i <= uint64_t(INT64_MAX)) { return NUMBER_ERROR; }
  }

  return negative ? (~i+1) : i;
}

SIMDJSON_UNUSED simdjson_really_inline simdjson_result<double> parse_double(const uint8_t * src) noexcept {
  //
  // Check for minus sign
  //
  bool negative = (*src == '-');
  src += negative;

  //
  // Parse the integer part.
  //
  uint64_t i = 0;
  const uint8_t *p = src;
  p += parse_digit(*p, i);
  bool leading_zero = (i == 0);
  while (parse_digit(*p, i)) { p++; }
  // no integer digits, or 0123 (zero must be solo)
  if ( p == src || (leading_zero && p != src+1)) { return NUMBER_ERROR; }

  //
  // Parse the decimal part.
  //
  int64_t exponent = 0;
  bool overflow;
  if (simdjson_likely(*p == '.')) {
    p++;
    const uint8_t *start_decimal_digits = p;
    if (!parse_digit(*p, i)) { return NUMBER_ERROR; } // no decimal digits
    p++;
    while (parse_digit(*p, i)) { p++; }
    exponent = -(p - start_decimal_digits);

    // Overflow check. More than 19 digits (minus the decimal) may be overflow.
    overflow = p-src-1 > 19;
    if (simdjson_unlikely(overflow && leading_zero)) {
      // Skip leading 0.00000 and see if it still overflows
      const uint8_t *start_digits = src + 2;
      while (*start_digits == '0') { start_digits++; }
      overflow = start_digits-src > 19;
    }
  } else {
    overflow = p-src > 19;
  }

  //
  // Parse the exponent
  //
  if (*p == 'e' || *p == 'E') {
    p++;
    bool exp_neg = *p == '-';
    p += exp_neg || *p == '+';

    uint64_t exp = 0;
    const uint8_t *start_exp_digits = p;
    while (parse_digit(*p, exp)) { p++; }
    // no exp digits, or 20+ exp digits
    if (p-start_exp_digits == 0 || p-start_exp_digits > 19) { return NUMBER_ERROR; }

    exponent += exp_neg ? 0-exp : exp;
    overflow = overflow || exponent < FASTFLOAT_SMALLEST_POWER || exponent > FASTFLOAT_LARGEST_POWER;
  }

  //
  // Assemble (or slow-parse) the float
  //
  double d;
  if (simdjson_likely(!overflow)) {
    if (compute_float_64(exponent, i, negative, d)) { return d; }
  }
  if (!parse_float_strtod(src-negative, &d)) {
    return NUMBER_ERROR;
  }
  return d;
}
} //namespace {}
#endif // SIMDJSON_SKIPNUMBERPARSING

} // namespace numberparsing
} // namespace stage2
} // namespace SIMDJSON_IMPLEMENTATION
} // unnamed namespace
