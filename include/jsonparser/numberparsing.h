

static const double power_of_ten[] = {
    1e-308, 1e-307, 1e-306, 1e-305, 1e-304, 1e-303, 1e-302, 1e-301, 1e-300,
    1e-299, 1e-298, 1e-297, 1e-296, 1e-295, 1e-294, 1e-293, 1e-292, 1e-291,
    1e-290, 1e-289, 1e-288, 1e-287, 1e-286, 1e-285, 1e-284, 1e-283, 1e-282,
    1e-281, 1e-280, 1e-279, 1e-278, 1e-277, 1e-276, 1e-275, 1e-274, 1e-273,
    1e-272, 1e-271, 1e-270, 1e-269, 1e-268, 1e-267, 1e-266, 1e-265, 1e-264,
    1e-263, 1e-262, 1e-261, 1e-260, 1e-259, 1e-258, 1e-257, 1e-256, 1e-255,
    1e-254, 1e-253, 1e-252, 1e-251, 1e-250, 1e-249, 1e-248, 1e-247, 1e-246,
    1e-245, 1e-244, 1e-243, 1e-242, 1e-241, 1e-240, 1e-239, 1e-238, 1e-237,
    1e-236, 1e-235, 1e-234, 1e-233, 1e-232, 1e-231, 1e-230, 1e-229, 1e-228,
    1e-227, 1e-226, 1e-225, 1e-224, 1e-223, 1e-222, 1e-221, 1e-220, 1e-219,
    1e-218, 1e-217, 1e-216, 1e-215, 1e-214, 1e-213, 1e-212, 1e-211, 1e-210,
    1e-209, 1e-208, 1e-207, 1e-206, 1e-205, 1e-204, 1e-203, 1e-202, 1e-201,
    1e-200, 1e-199, 1e-198, 1e-197, 1e-196, 1e-195, 1e-194, 1e-193, 1e-192,
    1e-191, 1e-190, 1e-189, 1e-188, 1e-187, 1e-186, 1e-185, 1e-184, 1e-183,
    1e-182, 1e-181, 1e-180, 1e-179, 1e-178, 1e-177, 1e-176, 1e-175, 1e-174,
    1e-173, 1e-172, 1e-171, 1e-170, 1e-169, 1e-168, 1e-167, 1e-166, 1e-165,
    1e-164, 1e-163, 1e-162, 1e-161, 1e-160, 1e-159, 1e-158, 1e-157, 1e-156,
    1e-155, 1e-154, 1e-153, 1e-152, 1e-151, 1e-150, 1e-149, 1e-148, 1e-147,
    1e-146, 1e-145, 1e-144, 1e-143, 1e-142, 1e-141, 1e-140, 1e-139, 1e-138,
    1e-137, 1e-136, 1e-135, 1e-134, 1e-133, 1e-132, 1e-131, 1e-130, 1e-129,
    1e-128, 1e-127, 1e-126, 1e-125, 1e-124, 1e-123, 1e-122, 1e-121, 1e-120,
    1e-119, 1e-118, 1e-117, 1e-116, 1e-115, 1e-114, 1e-113, 1e-112, 1e-111,
    1e-110, 1e-109, 1e-108, 1e-107, 1e-106, 1e-105, 1e-104, 1e-103, 1e-102,
    1e-101, 1e-100, 1e-99,  1e-98,  1e-97,  1e-96,  1e-95,  1e-94,  1e-93,
    1e-92,  1e-91,  1e-90,  1e-89,  1e-88,  1e-87,  1e-86,  1e-85,  1e-84,
    1e-83,  1e-82,  1e-81,  1e-80,  1e-79,  1e-78,  1e-77,  1e-76,  1e-75,
    1e-74,  1e-73,  1e-72,  1e-71,  1e-70,  1e-69,  1e-68,  1e-67,  1e-66,
    1e-65,  1e-64,  1e-63,  1e-62,  1e-61,  1e-60,  1e-59,  1e-58,  1e-57,
    1e-56,  1e-55,  1e-54,  1e-53,  1e-52,  1e-51,  1e-50,  1e-49,  1e-48,
    1e-47,  1e-46,  1e-45,  1e-44,  1e-43,  1e-42,  1e-41,  1e-40,  1e-39,
    1e-38,  1e-37,  1e-36,  1e-35,  1e-34,  1e-33,  1e-32,  1e-31,  1e-30,
    1e-29,  1e-28,  1e-27,  1e-26,  1e-25,  1e-24,  1e-23,  1e-22,  1e-21,
    1e-20,  1e-19,  1e-18,  1e-17,  1e-16,  1e-15,  1e-14,  1e-13,  1e-12,
    1e-11,  1e-10,  1e-9,   1e-8,   1e-7,   1e-6,   1e-5,   1e-4,   1e-3,
    1e-2,   1e-1,   1e0,    1e1,    1e2,    1e3,    1e4,    1e5,    1e6,
    1e7,    1e8,    1e9,    1e10,   1e11,   1e12,   1e13,   1e14,   1e15,
    1e16,   1e17,   1e18,   1e19,   1e20,   1e21,   1e22,   1e23,   1e24,
    1e25,   1e26,   1e27,   1e28,   1e29,   1e30,   1e31,   1e32,   1e33,
    1e34,   1e35,   1e36,   1e37,   1e38,   1e39,   1e40,   1e41,   1e42,
    1e43,   1e44,   1e45,   1e46,   1e47,   1e48,   1e49,   1e50,   1e51,
    1e52,   1e53,   1e54,   1e55,   1e56,   1e57,   1e58,   1e59,   1e60,
    1e61,   1e62,   1e63,   1e64,   1e65,   1e66,   1e67,   1e68,   1e69,
    1e70,   1e71,   1e72,   1e73,   1e74,   1e75,   1e76,   1e77,   1e78,
    1e79,   1e80,   1e81,   1e82,   1e83,   1e84,   1e85,   1e86,   1e87,
    1e88,   1e89,   1e90,   1e91,   1e92,   1e93,   1e94,   1e95,   1e96,
    1e97,   1e98,   1e99,   1e100,  1e101,  1e102,  1e103,  1e104,  1e105,
    1e106,  1e107,  1e108,  1e109,  1e110,  1e111,  1e112,  1e113,  1e114,
    1e115,  1e116,  1e117,  1e118,  1e119,  1e120,  1e121,  1e122,  1e123,
    1e124,  1e125,  1e126,  1e127,  1e128,  1e129,  1e130,  1e131,  1e132,
    1e133,  1e134,  1e135,  1e136,  1e137,  1e138,  1e139,  1e140,  1e141,
    1e142,  1e143,  1e144,  1e145,  1e146,  1e147,  1e148,  1e149,  1e150,
    1e151,  1e152,  1e153,  1e154,  1e155,  1e156,  1e157,  1e158,  1e159,
    1e160,  1e161,  1e162,  1e163,  1e164,  1e165,  1e166,  1e167,  1e168,
    1e169,  1e170,  1e171,  1e172,  1e173,  1e174,  1e175,  1e176,  1e177,
    1e178,  1e179,  1e180,  1e181,  1e182,  1e183,  1e184,  1e185,  1e186,
    1e187,  1e188,  1e189,  1e190,  1e191,  1e192,  1e193,  1e194,  1e195,
    1e196,  1e197,  1e198,  1e199,  1e200,  1e201,  1e202,  1e203,  1e204,
    1e205,  1e206,  1e207,  1e208,  1e209,  1e210,  1e211,  1e212,  1e213,
    1e214,  1e215,  1e216,  1e217,  1e218,  1e219,  1e220,  1e221,  1e222,
    1e223,  1e224,  1e225,  1e226,  1e227,  1e228,  1e229,  1e230,  1e231,
    1e232,  1e233,  1e234,  1e235,  1e236,  1e237,  1e238,  1e239,  1e240,
    1e241,  1e242,  1e243,  1e244,  1e245,  1e246,  1e247,  1e248,  1e249,
    1e250,  1e251,  1e252,  1e253,  1e254,  1e255,  1e256,  1e257,  1e258,
    1e259,  1e260,  1e261,  1e262,  1e263,  1e264,  1e265,  1e266,  1e267,
    1e268,  1e269,  1e270,  1e271,  1e272,  1e273,  1e274,  1e275,  1e276,
    1e277,  1e278,  1e279,  1e280,  1e281,  1e282,  1e283,  1e284,  1e285,
    1e286,  1e287,  1e288,  1e289,  1e290,  1e291,  1e292,  1e293,  1e294,
    1e295,  1e296,  1e297,  1e298,  1e299,  1e300,  1e301,  1e302,  1e303,
    1e304,  1e305,  1e306,  1e307,  1e308};

really_inline bool parse_number(const u8 *buf, UNUSED size_t len,
                                ParsedJson &pj, u32 depth, u32 offset,
                                bool found_zero, bool found_minus) {
  if (found_minus) {
    offset++;
    found_zero = (buf[offset] == '0');
  }
  const u8 *src = &buf[offset];
  // this can read past the string content, so we need to have overallocated
  m256 v = _mm256_loadu_si256((const m256 *)(src));
  u64 error_sump = 0;

  // categories to extract
  // Digits:
  // 0 (0x30) - bucket 0
  // 1-9 (never any distinction except if we didn't get the free kick at 0 due
  // to the leading minus) (0x31-0x39) - bucket 1
  // . (0x2e) - bucket 2
  // E or e - no distinction (0x45/0x65) - bucket 3
  // + (0x2b) - bucket 4
  // - (0x2d) - bucket 4
  // Terminators
  // Whitespace: 0x20, 0x09, 0x0a, 0x0d - bucket 5+6
  // Comma and the closes: 0x2c is comma, } is 0x5d, ] is 0x7d - bucket 5+7
  // Geoff suggests that this is not ideal, but it seems to work well enough.
  const m256 low_nibble_mask = _mm256_setr_epi8(
      //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
      33, 2, 2, 2, 2, 10, 2, 2, 2, 66, 64, 16, 32, 0xd0, 4, 0, 33, 2, 2, 2, 2,
      10, 2, 2, 2, 66, 64, 16, 32, 0xd0, 4, 0);
  const m256 high_nibble_mask = _mm256_setr_epi8(
      //  0   1   2   3   4   5   6   7   8   9   a   b   c   d   e   f
      64, 0, 52, 3, 8, -128, 8, 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 64, 0, 52, 3, 8,
      -128, 8, 0x80, 0, 0, 0, 0, 0, 0, 0, 0);

  m256 tmp = _mm256_and_si256(
      _mm256_shuffle_epi8(low_nibble_mask, v),
      _mm256_shuffle_epi8(
          high_nibble_mask,
          _mm256_and_si256(_mm256_srli_epi32(v, 4), _mm256_set1_epi8(0x7f))));

  m256 enders_mask = _mm256_set1_epi8(0xe0);
  m256 tmp_enders = _mm256_cmpeq_epi8(_mm256_and_si256(tmp, enders_mask),
                                      _mm256_set1_epi8(0));
  u32 enders = ~(u32)_mm256_movemask_epi8(tmp_enders);
  if (enders == 0) {
    // if there are no ender characters then we are using more than 31 bytes for
    // the number, and we can safely assume that it is a floating-point numbers
    // hopefully, this is uncommon and we can fall back on the standard API
    char *end;
    double result = strtod((const char *)src, &end);
    if ((errno != 0) || (end == (const char *)src) ||
        is_not_structural_or_whitespace(*end)) {
      return false;
    }
    if (found_minus) {
      result = -result;
    }
    pj.write_tape_double(depth, result);
    return true;
  }
  ///////////
  // From this point forward, we know that the
  // the number fits in 31 bytes.
  ///////////

  // number_mask captures everything before the first ender
  u32 number_mask = ~enders & (enders - 1);
  // let us identify just the digits 0-9
  m256 d_mask = _mm256_set1_epi8(0x03);
  m256 tmp_d =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, d_mask), _mm256_set1_epi8(0));
  u32 digit_characters = ~(u32)_mm256_movemask_epi8(tmp_d);
  digit_characters &= number_mask;
  // integers are probably common, so let us do them right away
  if (number_mask == digit_characters) {
    int stringlength = __builtin_ctz(~digit_characters);
    const char *end = (const char *)src + stringlength;
    u64 result = naivestrtoll((const char *)src, end);
    if (found_minus) {
      result = -result;
    }
    pj.write_tape_s64(depth, result);
    // it is valid as long as it does not start with zero!
    // or just 0, whether -0 is allowed is debatable?
    return ! ((found_zero)  && (stringlength > 1));
  }

  m256 n_mask = _mm256_set1_epi8(0x1f);
  m256 tmp_n =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, n_mask), _mm256_set1_epi8(0));
  u32 number_characters = ~(u32)_mm256_movemask_epi8(tmp_n);

  // put something into our error sump if we have something
  // before our ending characters that isn't a valid character
  // for the inside of our JSON
  number_characters &= number_mask;
  error_sump |= number_characters ^ number_mask;

  // we can now assume that all of the content made of relevant characters

  m256 p_mask = _mm256_set1_epi8(0x04);
  m256 tmp_p =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, p_mask), _mm256_set1_epi8(0));
  u32 decimal_characters = ~(u32)_mm256_movemask_epi8(tmp_p);
  decimal_characters &= number_mask;

  // the decimal character must be unique or absent
  // we might have 1e10, 0.1, ...
  error_sump |= ((decimal_characters) & (decimal_characters - 1));

  // detect the exponential characters
  m256 e_mask = _mm256_set1_epi8(0x08);
  m256 tmp_e =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, e_mask), _mm256_set1_epi8(0));
  u32 exponent_characters = ~(u32)_mm256_movemask_epi8(tmp_e);
  exponent_characters &= number_mask;

  // the exponent character must be unique or absent
  error_sump |= ((exponent_characters) & (exponent_characters - 1));

  // if they exist the exponent character must follow the decimal_characters
  // character
  error_sump |=
      ((exponent_characters - 1) & decimal_characters) ^ decimal_characters;

  // if the  zero character is in first position, it
  // needs to be followed by the decimal
  error_sump |= found_zero ^ ((decimal_characters >> 1) & found_zero);

  // let us detect the sign characters
  m256 s_mask = _mm256_set1_epi8(0x10);
  m256 tmp_s =
      _mm256_cmpeq_epi8(_mm256_and_si256(tmp, s_mask), _mm256_set1_epi8(0));

  u32 sign_characters = ~(u32)_mm256_movemask_epi8(tmp_s);
  sign_characters &= number_mask;

  // any sign character must be followed by a digit
  error_sump |= (~(digit_characters >> 1)) & sign_characters;

  // there is at most one sign character
  error_sump |= ((sign_characters) & (sign_characters - 1));

  // the exponent must be followed by either a sign character or a digit
  error_sump |=
      (~((digit_characters | sign_characters) >> 1)) & exponent_characters;


  if(error_sump != 0) {
      return false;
  }
  // so we have a nice float-point at this time

  const char *p = (const char *)src;
  // we start with digits followed by "." or "e" or "E".
  // scan them
  int integerpartlength =
      __builtin_ctz(exponent_characters | decimal_characters);
  const char *endjustinteger = p + integerpartlength;
  uint64_t integerpart = *p - '0'; // there must be at least one digit
  p++;
  for (; p != endjustinteger; p++) {
    integerpart = (integerpart * 10) + (*p - '0');
  }
  double result = integerpart;
  if (decimal_characters != 0) {
    p++;
    int mantissalength = __builtin_ctz(exponent_characters | enders);
    const char *endmantissa = (const char *)src + mantissalength;
    int fracdigitcount = endmantissa - p; // could be zero!
    uint64_t fractionalpart = 0;          // there could be nothing
    for (; p != endmantissa; p++) {
      fractionalpart = (fractionalpart * 10) + (*p - '0');
    }
    result += fractionalpart * power_of_ten[308 - fracdigitcount];
  }
  if (exponent_characters != 0) {
    p++; // skip exponent
    int sign = +1;
    if (p[0] == '+')
      p++;
    if (p[0] == '-') {
      p++;
      sign = -1;
    }
    int stringlength = __builtin_ctz(~number_mask);
    const char *endnumber = (const char *)src + stringlength;

    uint64_t exppart = *p - '0';
    p++;
    for (; p != endnumber; p++) {
      exppart = (exppart * 10) + (*p - '0');
    }
    if (exppart > 308) {
      return false;
    }
    // betting that these branches are highly predictible
    // could use arithmetic instead
    if (sign == 1) {
      result = result * power_of_ten[308 + exppart];
    } else {
      result = result * power_of_ten[308 - exppart];
    }
  }
  pj.write_tape_double(depth, result);
  return true;
}
