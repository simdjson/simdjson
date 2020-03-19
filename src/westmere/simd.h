#ifndef SIMDJSON_WESTMERE_SIMD_H
#define SIMDJSON_WESTMERE_SIMD_H

#include "simdjson.h"
#include "westmere/intrinsics.h"



TARGET_WESTMERE
namespace simdjson::westmere::simd {

///////////////////////
// The following tables should probably be moved off somewhere else.
///////////////////////

// 256 * 8 bytes = 2kB, easily fits in cache.
static uint64_t thintable_epi8[256]={
0x0706050403020100,0x0007060504030201,0x0007060504030200,0x0000070605040302,
0x0007060504030100,0x0000070605040301,0x0000070605040300,0x0000000706050403,
0x0007060504020100,0x0000070605040201,0x0000070605040200,0x0000000706050402,
0x0000070605040100,0x0000000706050401,0x0000000706050400,0x0000000007060504,
0x0007060503020100,0x0000070605030201,0x0000070605030200,0x0000000706050302,
0x0000070605030100,0x0000000706050301,0x0000000706050300,0x0000000007060503,
0x0000070605020100,0x0000000706050201,0x0000000706050200,0x0000000007060502,
0x0000000706050100,0x0000000007060501,0x0000000007060500,0x0000000000070605,
0x0007060403020100,0x0000070604030201,0x0000070604030200,0x0000000706040302,
0x0000070604030100,0x0000000706040301,0x0000000706040300,0x0000000007060403,
0x0000070604020100,0x0000000706040201,0x0000000706040200,0x0000000007060402,
0x0000000706040100,0x0000000007060401,0x0000000007060400,0x0000000000070604,
0x0000070603020100,0x0000000706030201,0x0000000706030200,0x0000000007060302,
0x0000000706030100,0x0000000007060301,0x0000000007060300,0x0000000000070603,
0x0000000706020100,0x0000000007060201,0x0000000007060200,0x0000000000070602,
0x0000000007060100,0x0000000000070601,0x0000000000070600,0x0000000000000706,
0x0007050403020100,0x0000070504030201,0x0000070504030200,0x0000000705040302,
0x0000070504030100,0x0000000705040301,0x0000000705040300,0x0000000007050403,
0x0000070504020100,0x0000000705040201,0x0000000705040200,0x0000000007050402,
0x0000000705040100,0x0000000007050401,0x0000000007050400,0x0000000000070504,
0x0000070503020100,0x0000000705030201,0x0000000705030200,0x0000000007050302,
0x0000000705030100,0x0000000007050301,0x0000000007050300,0x0000000000070503,
0x0000000705020100,0x0000000007050201,0x0000000007050200,0x0000000000070502,
0x0000000007050100,0x0000000000070501,0x0000000000070500,0x0000000000000705,
0x0000070403020100,0x0000000704030201,0x0000000704030200,0x0000000007040302,
0x0000000704030100,0x0000000007040301,0x0000000007040300,0x0000000000070403,
0x0000000704020100,0x0000000007040201,0x0000000007040200,0x0000000000070402,
0x0000000007040100,0x0000000000070401,0x0000000000070400,0x0000000000000704,
0x0000000703020100,0x0000000007030201,0x0000000007030200,0x0000000000070302,
0x0000000007030100,0x0000000000070301,0x0000000000070300,0x0000000000000703,
0x0000000007020100,0x0000000000070201,0x0000000000070200,0x0000000000000702,
0x0000000000070100,0x0000000000000701,0x0000000000000700,0x0000000000000007,
0x0006050403020100,0x0000060504030201,0x0000060504030200,0x0000000605040302,
0x0000060504030100,0x0000000605040301,0x0000000605040300,0x0000000006050403,
0x0000060504020100,0x0000000605040201,0x0000000605040200,0x0000000006050402,
0x0000000605040100,0x0000000006050401,0x0000000006050400,0x0000000000060504,
0x0000060503020100,0x0000000605030201,0x0000000605030200,0x0000000006050302,
0x0000000605030100,0x0000000006050301,0x0000000006050300,0x0000000000060503,
0x0000000605020100,0x0000000006050201,0x0000000006050200,0x0000000000060502,
0x0000000006050100,0x0000000000060501,0x0000000000060500,0x0000000000000605,
0x0000060403020100,0x0000000604030201,0x0000000604030200,0x0000000006040302,
0x0000000604030100,0x0000000006040301,0x0000000006040300,0x0000000000060403,
0x0000000604020100,0x0000000006040201,0x0000000006040200,0x0000000000060402,
0x0000000006040100,0x0000000000060401,0x0000000000060400,0x0000000000000604,
0x0000000603020100,0x0000000006030201,0x0000000006030200,0x0000000000060302,
0x0000000006030100,0x0000000000060301,0x0000000000060300,0x0000000000000603,
0x0000000006020100,0x0000000000060201,0x0000000000060200,0x0000000000000602,
0x0000000000060100,0x0000000000000601,0x0000000000000600,0x0000000000000006,
0x0000050403020100,0x0000000504030201,0x0000000504030200,0x0000000005040302,
0x0000000504030100,0x0000000005040301,0x0000000005040300,0x0000000000050403,
0x0000000504020100,0x0000000005040201,0x0000000005040200,0x0000000000050402,
0x0000000005040100,0x0000000000050401,0x0000000000050400,0x0000000000000504,
0x0000000503020100,0x0000000005030201,0x0000000005030200,0x0000000000050302,
0x0000000005030100,0x0000000000050301,0x0000000000050300,0x0000000000000503,
0x0000000005020100,0x0000000000050201,0x0000000000050200,0x0000000000000502,
0x0000000000050100,0x0000000000000501,0x0000000000000500,0x0000000000000005,
0x0000000403020100,0x0000000004030201,0x0000000004030200,0x0000000000040302,
0x0000000004030100,0x0000000000040301,0x0000000000040300,0x0000000000000403,
0x0000000004020100,0x0000000000040201,0x0000000000040200,0x0000000000000402,
0x0000000000040100,0x0000000000000401,0x0000000000000400,0x0000000000000004,
0x0000000003020100,0x0000000000030201,0x0000000000030200,0x0000000000000302,
0x0000000000030100,0x0000000000000301,0x0000000000000300,0x0000000000000003,
0x0000000000020100,0x0000000000000201,0x0000000000000200,0x0000000000000002,
0x0000000000000100,0x0000000000000001,0x0000000000000000,0x0000000000000000,
};

static const uint8_t pshufb_combine_table[272] = {
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,
	0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x80,
	0x00,0x01,0x02,0x03,0x04,0x05,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x80,0x80,
	0x00,0x01,0x02,0x03,0x04,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x80,0x80,0x80,
	0x00,0x01,0x02,0x03,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x80,0x80,0x80,0x80,
	0x00,0x01,0x02,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x80,0x80,0x80,0x80,0x80,
	0x00,0x01,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x80,0x80,0x80,0x80,0x80,0x80,
	0x00,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
	0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,
};
// table modified and copied from http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetTable
static const unsigned char BitsSetTable256mul2[256] = 
{
#   define B2(n) (n)*2,     (n)*2+2,     (n)*2+2,     (n)*2+4
#   define B4(n) B2(n), B2(n+1), B2(n+1), B2(n+2)
#   define B6(n) B4(n), B4(n+1), B4(n+1), B4(n+2)
    B6(0), B6(1), B6(1), B6(2)
#undef B2
#undef B4
#undef B6
};

  template<typename Child>
  struct base {
    __m128i value;

    // Zero constructor
    really_inline base() : value{__m128i()} {}

    // Conversion from SIMD register
    really_inline base(const __m128i _value) : value(_value) {}

    // Conversion to SIMD register
    really_inline operator const __m128i&() const { return this->value; }
    really_inline operator __m128i&() { return this->value; }

    // Bit operations
    really_inline Child operator|(const Child other) const { return _mm_or_si128(*this, other); }
    really_inline Child operator&(const Child other) const { return _mm_and_si128(*this, other); }
    really_inline Child operator^(const Child other) const { return _mm_xor_si128(*this, other); }
    really_inline Child bit_andnot(const Child other) const { return _mm_andnot_si128(other, *this); }
    really_inline Child& operator|=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast | other; return *this_cast; }
    really_inline Child& operator&=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast & other; return *this_cast; }
    really_inline Child& operator^=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    really_inline base8() : base<simd8<T>>() {}
    really_inline base8(const __m128i _value) : base<simd8<T>>(_value) {}

    really_inline Mask operator==(const simd8<T> other) const { return _mm_cmpeq_epi8(*this, other); }

    static const int SIZE = sizeof(base<simd8<T>>::value);

    template<int N=1>
    really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm_alignr_epi8(*this, prev_chunk, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static really_inline simd8<bool> splat(bool _value) { return _mm_set1_epi8(-(!!_value)); }

    really_inline simd8<bool>() : base8() {}
    really_inline simd8<bool>(const __m128i _value) : base8<bool>(_value) {}
    // Splat constructor
    really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}

    really_inline int to_bitmask() const { return _mm_movemask_epi8(*this); }
    really_inline bool any() const { return !_mm_testz_si128(*this, *this); }
    really_inline simd8<bool> operator~() const { return *this ^ true; }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static really_inline simd8<T> splat(T _value) { return _mm_set1_epi8(_value); }
    static really_inline simd8<T> zero() { return _mm_setzero_si128(); }
    static really_inline simd8<T> load(const T values[16]) {
      return _mm_loadu_si128(reinterpret_cast<const __m128i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static really_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    really_inline base8_numeric() : base8<T>() {}
    really_inline base8_numeric(const __m128i _value) : base8<T>(_value) {}

    // Store to array
    really_inline void store(T dst[16]) const { return _mm_storeu_si128(reinterpret_cast<__m128i *>(dst), *this); }

    // Override to distinguish from bool version
    really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

    // Addition/subtraction are the same for signed and unsigned
    really_inline simd8<T> operator+(const simd8<T> other) const { return _mm_add_epi8(*this, other); }
    really_inline simd8<T> operator-(const simd8<T> other) const { return _mm_sub_epi8(*this, other); }
    really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *(simd8<T>*)this; }
    really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *(simd8<T>*)this; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm_shuffle_epi8(lookup_table, *this);
    }
    // Return a new value where all bytes corresponding to a one in the mask (interpreted as a bitset)
    // have been omitted. Only the first count_ones(mask) bytes of the result are significant. 
    template<typename L>
    really_inline simd8<L> compress(uint16_t mask) const {
      // this particular implementation was inspired by work done by @animetosho
      // we do it in two steps, first 8 bytes and then second 8 bytes
      uint8_t mask1 = static_cast<uint8_t>(mask); // least significant 8 bits
      uint8_t mask2 = static_cast<uint8_t>(mask >> 8); // most significant 8 bits
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register, using only
      // two instructions on most compilers.
      __m128i shufmask =  _mm_set_epi64x(thintable_epi8[mask1], thintable_epi8[mask2]);
      // we increment by 0x08 the second half of the mask
      shufmask =
      _mm_add_epi8(shufmask, _mm_set_epi32(0x08080808, 0x08080808, 0, 0));
      // this is the version "nearly pruned"
      __m128i pruned = _mm_shuffle_epi8(*this, shufmask);
      // we still need to put the two halves together.
      // we compute the popcount of the first half:
      int pop1 = BitsSetTable256mul2[mask1];
      // then load the corresponding mask
      __m128i compactmask =
      _mm_loadu_si128((const __m128i *)(pshufb_combine_table + pop1 * 8));
      return _mm_shuffle_epi8(pruned, compactmask);
    }

    template<typename L>
    really_inline simd8<L> lookup_16(
        L replace0,  L replace1,  L replace2,  L replace3,
        L replace4,  L replace5,  L replace6,  L replace7,
        L replace8,  L replace9,  L replace10, L replace11,
        L replace12, L replace13, L replace14, L replace15) const {
      return lookup_16(simd8<L>::repeat_16(
        replace0,  replace1,  replace2,  replace3,
        replace4,  replace5,  replace6,  replace7,
        replace8,  replace9,  replace10, replace11,
        replace12, replace13, replace14, replace15
      ));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> : base8_numeric<int8_t> {
    really_inline simd8() : base8_numeric<int8_t>() {}
    really_inline simd8(const __m128i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(_mm_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Order-sensitive comparisons
    really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return _mm_max_epi8(*this, other); }
    really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return _mm_min_epi8(*this, other); }
    really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm_cmpgt_epi8(*this, other); }
    really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return _mm_cmpgt_epi8(other, *this); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    really_inline simd8() : base8_numeric<uint8_t>() {}
    really_inline simd8(const __m128i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const uint8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(_mm_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Saturated math
    really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm_adds_epu8(*this, other); }
    really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm_subs_epu8(*this, other); }

    // Order-specific operations
    really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return _mm_max_epu8(*this, other); }
    really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return _mm_min_epu8(*this, other); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max(*this) == other; }
    really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min(*this) == other; }
    really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }

    // Bit-specific operations
    really_inline simd8<bool> bits_not_set() const { return *this == uint8_t(0); }
    really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    really_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }
    really_inline bool bits_not_set_anywhere() const { return _mm_testz_si128(*this, *this); }
    really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return _mm_testz_si128(*this, bits); }
    really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    really_inline int get_bit() const { return _mm_movemask_epi8(_mm_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static const int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    const simd8<T> chunks[NUM_CHUNKS];

    really_inline simd8x64() : chunks{simd8<T>(), simd8<T>(), simd8<T>(), simd8<T>()} {}
    really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+16), simd8<T>::load(ptr+32), simd8<T>::load(ptr+48)} {}

    really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3);
    }

    template <typename F>
    static really_inline void each_index(F const& each) {
      each(0);
      each(1);
      each(2);
      each(3);
    }

    template <typename F>
    really_inline void each(F const& each_chunk) const
    {
      each_chunk(this->chunks[0]);
      each_chunk(this->chunks[1]);
      each_chunk(this->chunks[2]);
      each_chunk(this->chunks[3]);
    }

    template <typename F, typename R=bool>
    really_inline simd8x64<R> map(F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0]),
        map_chunk(this->chunks[1]),
        map_chunk(this->chunks[2]),
        map_chunk(this->chunks[3])
      );
    }

    template <typename F, typename R=bool>
    really_inline simd8x64<R> map(const simd8x64<uint8_t> b, F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0], b.chunks[0]),
        map_chunk(this->chunks[1], b.chunks[1]),
        map_chunk(this->chunks[2], b.chunks[2]),
        map_chunk(this->chunks[3], b.chunks[3])
      );
    }

    template <typename F>
    really_inline simd8<T> reduce(F const& reduce_pair) const {
      return reduce_pair(
        reduce_pair(this->chunks[0], this->chunks[1]),
        reduce_pair(this->chunks[2], this->chunks[3])
      );
    }

    really_inline uint64_t to_bitmask() const {
      uint64_t r0 = static_cast<uint32_t>(this->chunks[0].to_bitmask());
      uint64_t r1 =                       this->chunks[1].to_bitmask();
      uint64_t r2 =                       this->chunks[2].to_bitmask();
      uint64_t r3 =                       this->chunks[3].to_bitmask();
      return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
    }

    really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a | mask; } );
    }

    really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a == mask; } ).to_bitmask();
    }

    really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->map( [&](auto a) { return a <= mask; } ).to_bitmask();
    }
  }; // struct simd8x64<T>

} // namespace simdjson::westmere::simd
UNTARGET_REGION

#endif // SIMDJSON_WESTMERE_SIMD_INPUT_H
