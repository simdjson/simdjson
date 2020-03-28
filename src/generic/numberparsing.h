namespace numberparsing {


// Attempts to compute i * 10^(power) exactly; and if "negative" is
// true, negate the result.
// This function will only work in some cases, when it does not work, success is
// set to false. This should work *most of the time* (like 99% of the time).
// We assume that power is in the [FASTFLOAT_SMALLEST_POWER,
// FASTFLOAT_LARGEST_POWER] interval: the caller is responsible for this check.
really_inline double compute_float_64(int64_t power, uint64_t i, bool negative,
                                      bool *success) {
  // we start with a fast path
  // It was described in
  // Clinger WD. How to read floating point numbers accurately.
  // ACM SIGPLAN Notices. 1990
  if (-22 <= power && power <= 22 && i <= 9007199254740991) {
    // convert the integer into a double. This is lossless since
    // 0 <= i <= 2^53 - 1.
    double d = i;
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
    *success = true;
    return d;
  }
  // When 22 < power && power <  22 + 16, we could
  // hope for another, secondary fast path.  It wa
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
    return 0.0;
  }

  // We are going to need to do some 64-bit arithmetic to get a more precise product.
  // We use a table lookup approach.
  components c =
      power_of_ten_components[power - FASTFLOAT_SMALLEST_POWER];
      // safe because
      // power >= FASTFLOAT_SMALLEST_POWER
      // and power <= FASTFLOAT_LARGEST_POWER
  // we recover the mantissa of the power, it has a leading 1. It is always
  // rounded down.
  uint64_t factor_mantissa = c.mantissa;

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
  if (unlikely((upper & 0x1FF) == 0x1FF) && (lower + i < lower)) {
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
      *success = false;
      return 0;
    }
    upper = product_high;
    lower = product_middle;
  }
  // The final mantissa should be 53 bits with a leading 1.
  // We shift it so that it occupies 54 bits with a leading 1.
  ///////
  uint64_t upperbit = upper >> 63;
  uint64_t mantissa = upper >> (upperbit + 9);
  lz += 1 ^ upperbit;

  // Here we have mantissa < (1<<54).

  // We have to round to even. The "to even" part
  // is only a problem when we are right in between two floats
  // which we guard against.
  // If we have lots of trailing zeros, we may fall right between two
  // floating-point values.
  if (unlikely((lower == 0) && ((upper & 0x1FF) == 0) &&
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
      *success = false;
      return 0;
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
  uint64_t real_exponent = c.exp - lz;
  // we have to check that real_exponent is in range, otherwise we bail out
  if (unlikely((real_exponent < 1) || (real_exponent > 2046))) {
    *success = false;
    return 0;
  }
  mantissa |= real_exponent << 52;
  mantissa |= (((uint64_t)negative) << 63);
  double d;
  memcpy(&d, &mantissa, sizeof(d));
  *success = true;
  return d;
}

static bool parse_float_strtod(const char *ptr, double *outDouble) {
  char *endptr;
  *outDouble = strtod(ptr, &endptr);
  // Some libraries will set errno = ERANGE when the value is subnormal,
  // yet we may want to be able to parse subnormal values.
  // However, we do not want to tolerate NAN or infinite values.
  //
  // Values like infinity or NaN are not allowed in the JSON specification.
  // If you consume a large value and you map it to "infinity", you will no
  // longer be able to serialize back a standard-compliant JSON. And there is
  // no realistic application where you might need values so large than they
  // can't fit in binary64. The maximal value is about  1.7976931348623157 ×
  // 10^308 It is an unimaginable large number. There will never be any piece of
  // engineering involving as many as 10^308 parts. It is estimated that there
  // are about 10^80 atoms in the universe.  The estimate for the total number
  // of electrons is similar. Using a double-precision floating-point value, we
  // can represent easily the number of atoms in the universe. We could  also
  // represent the number of ways you can pick any three individual atoms at
  // random in the universe. If you ever encounter a number much larger than
  // 10^308, you know that you have a bug. RapidJSON will reject a document with
  // a float that does not fit in binary64. JSON for Modern C++ (nlohmann/json)
  // will flat out throw an exception.
  //
  if ((endptr == ptr) || (!std::isfinite(*outDouble))) {
    return false;
  }
  return true;
}

really_inline bool is_integer(char c) {
  return (c >= '0' && c <= '9');
  // this gets compiled to (uint8_t)(c - '0') <= 9 on all decent compilers
}

// We need to check that the character following a zero is valid. This is
// probably frequent and it is harder than it looks. We are building all of this
// just to differentiate between 0x1 (invalid), 0,1 (valid) 0e1 (valid)...
const bool structural_or_whitespace_or_exponent_or_decimal_negated[256] = {
    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};

really_inline bool
is_not_structural_or_whitespace_or_exponent_or_decimal(unsigned char c) {
  return structural_or_whitespace_or_exponent_or_decimal_negated[c];
}

// check quickly whether the next 8 chars are made of digits
// at a glance, it looks better than Mula's
// http://0x80.pl/articles/swar-digits-validate.html
really_inline bool is_made_of_eight_digits_fast(const char *chars) {
  uint64_t val;
  // this can read up to 7 bytes beyond the buffer size, but we require
  // SIMDJSON_PADDING of padding
  static_assert(7 <= SIMDJSON_PADDING);
  memcpy(&val, chars, 8);
  // a branchy method might be faster:
  // return (( val & 0xF0F0F0F0F0F0F0F0 ) == 0x3030303030303030)
  //  && (( (val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0 ) ==
  //  0x3030303030303030);
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}

// called by parse_number when we know that the output is an integer,
// but where there might be some integer overflow.
// we want to catch overflows!
// Do not call this function directly as it skips some of the checks from
// parse_number
//
// This function will almost never be called!!!
//
never_inline bool parse_large_integer(const uint8_t *const src,
                                      parser &parser,
                                      bool found_minus) {
  const char *p = reinterpret_cast<const char *>(src);

  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
  }
  uint64_t i;
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    i = 0;
  } else {
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      if (mul_overflow(i, 10, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(src);
#endif
        return false; // overflow
      }
      if (add_overflow(i, digit, &i)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(src);
#endif
        return false; // overflow
      }
      ++p;
    }
  }
  if (negative) {
    if (i > 0x8000000000000000) {
      // overflows!
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(src);
#endif
      return false; // overflow
    } else if (i == 0x8000000000000000) {
      // In two's complement, we cannot represent 0x8000000000000000
      // as a positive signed integer, but the negative version is
      // possible.
      constexpr int64_t signed_answer = INT64_MIN;
      parser.on_number_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, src);
#endif
    } else {
      // we can negate safely
      int64_t signed_answer = -static_cast<int64_t>(i);
      parser.on_number_s64(signed_answer);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(signed_answer, src);
#endif
    }
  } else {
    // we have a positive integer, the contract is that
    // we try to represent it as a signed integer and only
    // fallback on unsigned integers if absolutely necessary.
    if (i < 0x8000000000000000) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_integer(i, src);
#endif
      parser.on_number_s64(i);
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_unsigned_integer(i, src);
#endif
      parser.on_number_u64(i);
    }
  }
  return is_structural_or_whitespace(*p);
}

bool slow_float_parsing(UNUSED const char * src, parser &parser) {
  double d;
  if (parse_float_strtod(src, &d)) {
    parser.on_number_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_float(d, (const uint8_t *)src);
#endif
    return true;
  }
#ifdef JSON_TEST_NUMBERS // for unit testing
  found_invalid_number((const uint8_t *)src);
#endif
  return false;
}

// parse the number at src
// define JSON_TEST_NUMBERS for unit testing
//
// It is assumed that the number is followed by a structural ({,},],[) character
// or a white space character. If that is not the case (e.g., when the JSON
// document is made of a single number), then it is necessary to copy the
// content and append a space before calling this function.
//
// Our objective is accurate parsing (ULP of 0) at high speed.
really_inline bool parse_number(UNUSED const uint8_t *const src,
                                UNUSED bool found_minus,
                                parser &parser) {
#ifdef SIMDJSON_SKIPNUMBERPARSING // for performance analysis, it is sometimes
                                  // useful to skip parsing
  parser.on_number_s64(0);        // always write zero
  return true;                    // always succeeds
#else
  const char *p = reinterpret_cast<const char *>(src);
  bool negative = false;
  if (found_minus) {
    ++p;
    negative = true;
    if (!is_integer(*p)) { // a negative sign must be followed by an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(src);
#endif
      return false;
    }
  }
  const char *const start_digits = p;

  uint64_t i;      // an unsigned int avoids signed overflows (which are bad)
  if (*p == '0') { // 0 cannot be followed by an integer
    ++p;
    if (is_not_structural_or_whitespace_or_exponent_or_decimal(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(src);
#endif
      return false;
    }
    i = 0;
  } else {
    if (!(is_integer(*p))) { // must start with an integer
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(src);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    i = digit;
    p++;
    // the is_made_of_eight_digits_fast routine is unlikely to help here because
    // we rarely see large integer parts like 123456789
    while (is_integer(*p)) {
      digit = *p - '0';
      // a multiplication by 10 is cheaper than an arbitrary integer
      // multiplication
      i = 10 * i + digit; // might overflow, we will handle the overflow later
      ++p;
    }
  }
  int64_t exponent = 0;
  bool is_float = false;
  if ('.' == *p) {
    is_float = true; // At this point we know that we have a float
    // we continue with the fiction that we have an integer. If the
    // floating point number is representable as x * 10^z for some integer
    // z that fits in 53 bits, then we will be able to convert back the
    // the integer into a float in a lossless manner.
    ++p;
    const char *const first_after_period = p;
    if (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // might overflow + multiplication by 10 is likely
                          // cheaper than arbitrary mult.
      // we will handle the overflow later
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(src);
#endif
      return false;
    }
#ifdef SWAR_NUMBER_PARSING
    // this helps if we have lots of decimals!
    // this turns out to be frequent enough.
    if (is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
    }
#endif
    while (is_integer(*p)) {
      unsigned char digit = *p - '0';
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
                          // because we have parse_highprecision_float later.
    }
    exponent = first_after_period - p;
  }
  int digit_count =
      p - start_digits - 1; // used later to guard against overflows
  int64_t exp_number = 0;   // exponential part
  if (('e' == *p) || ('E' == *p)) {
    is_float = true;
    ++p;
    bool neg_exp = false;
    if ('-' == *p) {
      neg_exp = true;
      ++p;
    } else if ('+' == *p) {
      ++p;
    }
    if (!is_integer(*p)) {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(src);
#endif
      return false;
    }
    unsigned char digit = *p - '0';
    exp_number = digit;
    p++;
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    if (is_integer(*p)) {
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    while (is_integer(*p)) {
      if (exp_number > 0x100000000) { // we need to check for overflows
                                      // we refuse to parse this
#ifdef JSON_TEST_NUMBERS // for unit testing
        found_invalid_number(src);
#endif
        return false;
      }
      digit = *p - '0';
      exp_number = 10 * exp_number + digit;
      ++p;
    }
    exponent += (neg_exp ? -exp_number : exp_number);
  }
  if (is_float) {
    // If we frequently had to deal with long strings of digits,
    // we could extend our code by using a 128-bit integer instead
    // of a 64-bit integer. However, this is uncommon in practice.
    if (unlikely((digit_count >= 19))) { // this is uncommon
      // It is possible that the integer had an overflow.
      // We have to handle the case where we have 0.0000somenumber.
      const char *start = start_digits;
      while ((*start == '0') || (*start == '.')) {
        start++;
      }
      // we over-decrement by one when there is a '.'
      digit_count -= (start - start_digits);
      if (digit_count >= 19) {
        // Ok, chances are good that we had an overflow!
        // this is almost never going to get called!!!
        // we start anew, going slowly!!!
        // This will happen in the following examples:
        // 10000000000000000000000000000000000000000000e+308
        // 3.1415926535897932384626433832795028841971693993751
        //
        return slow_float_parsing((const char *) src, parser);
      }
    }
    if (unlikely(exponent < FASTFLOAT_SMALLEST_POWER) ||
        (exponent > FASTFLOAT_LARGEST_POWER)) { // this is uncommon!!!
      // this is almost never going to get called!!!
      // we start anew, going slowly!!!
      return slow_float_parsing((const char *) src, parser);
    }
    bool success = true;
    double d = compute_float_64(exponent, i, negative, &success);
    if (!success) {
      // we are almost never going to get here.
      success = parse_float_strtod((const char *)src, &d);
    }
    if (success) {
      parser.on_number_double(d);
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_float(d, src);
#endif
      return true;
    } else {
#ifdef JSON_TEST_NUMBERS // for unit testing
      found_invalid_number(src);
#endif
      return false;
    }
  } else {
    if (unlikely(digit_count >= 18)) { // this is uncommon!!!
      // there is a good chance that we had an overflow, so we need
      // need to recover: we parse the whole thing again.
      return parse_large_integer(src, parser, found_minus);
    }
    i = negative ? 0 - i : i;
    parser.on_number_s64(i);
#ifdef JSON_TEST_NUMBERS // for unit testing
    found_integer(i, src);
#endif
  }
  return is_structural_or_whitespace(*p);
#endif // SIMDJSON_SKIPNUMBERPARSING
}

} // namespace numberparsing
