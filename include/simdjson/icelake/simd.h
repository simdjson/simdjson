#ifndef SIMDJSON_ICELAKE_SIMD_H
#define SIMDJSON_ICELAKE_SIMD_H

#include "simdjson/internal/simdprune_tables.h"

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace simd {

  // Forward-declared so they can be used by splat and friends.
  template<typename Child>
  struct base {
    __m512i value;

    // Zero constructor
    simdjson_really_inline base() : value{__m512i()} {}

    // Conversion from SIMD register
    simdjson_really_inline base(const __m512i _value) : value(_value) {}

    // Conversion to SIMD register
    simdjson_really_inline operator const __m512i&() const { return this->value; }
    simdjson_really_inline operator __m512i&() { return this->value; }

    // Bit operations
    simdjson_really_inline Child operator|(const Child other) const { return _mm512_or_si512(*this, other); }
    simdjson_really_inline Child operator&(const Child other) const { return _mm512_and_si512(*this, other); }
    simdjson_really_inline Child operator^(const Child other) const { return _mm512_xor_si512(*this, other); }
    simdjson_really_inline Child bit_andnot(const Child other) const { return _mm512_andnot_si512(other, *this); }
    simdjson_really_inline Child& operator|=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast | other; return *this_cast; }
    simdjson_really_inline Child& operator&=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast & other; return *this_cast; }
    simdjson_really_inline Child& operator^=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint32_t bitmask_t;
    typedef uint64_t bitmask2_t;

    simdjson_really_inline base8() : base<simd8<T>>() {}
    simdjson_really_inline base8(const __m512i _value) : base<simd8<T>>(_value) {}

    simdjson_really_inline uint64_t operator==(const simd8<T> other) const { return _mm512_cmpeq_epi8_mask(*this, other); }

    static const int SIZE = sizeof(base<T>::value);

    template<int N=1>
    simdjson_really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm512_alignr_epi8(*this, _mm512_permutex2var_epi64(prev_chunk, _mm512_set_epi64(13, 12, 11, 10, 9, 8, 7, 6), *this), 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static simdjson_really_inline simd8<bool> splat(bool _value) { return _mm512_set1_epi8(uint8_t(-(!!_value))); }

    simdjson_really_inline simd8<bool>() : base8() {}
    simdjson_really_inline simd8<bool>(const __m512i _value) : base8<bool>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}
    simdjson_really_inline bool any() const { return !!_mm512_test_epi8_mask (*this, *this); }
    simdjson_really_inline simd8<bool> operator~() const { return *this ^ true; }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static simdjson_really_inline simd8<T> splat(T _value) { return _mm512_set1_epi8(_value); }
    static simdjson_really_inline simd8<T> zero() { return _mm512_setzero_si512(); }
    static simdjson_really_inline simd8<T> load(const T values[64]) {
      return _mm512_loadu_si512(reinterpret_cast<const __m512i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static simdjson_really_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    simdjson_really_inline base8_numeric() : base8<T>() {}
    simdjson_really_inline base8_numeric(const __m512i _value) : base8<T>(_value) {}

    // Store to array
    simdjson_really_inline void store(T dst[64]) const { return _mm512_storeu_si512(reinterpret_cast<__m512i *>(dst), *this); }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_really_inline simd8<T> operator+(const simd8<T> other) const { return _mm512_add_epi8(*this, other); }
    simdjson_really_inline simd8<T> operator-(const simd8<T> other) const { return _mm512_sub_epi8(*this, other); }
    simdjson_really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *static_cast<simd8<T>*>(this); }
    simdjson_really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *static_cast<simd8<T>*>(this); }

    // Override to distinguish from bool version
    simdjson_really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm512_shuffle_epi8(lookup_table, *this);
    }

    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 32 - count_ones(mask) bytes of the result are significant but 32 bytes
    // get written.
    // Design consideration: it seems like a function with the
    // signature simd8<L> compress(uint32_t mask) would be
    // sensible, but the AVX ISA makes this kind of approach difficult.
    template<typename L>
    simdjson_really_inline void compress(uint64_t mask, L * output) const {
      _mm512_mask_compressstoreu_epi8 (output,~mask,*this);
    }

    template<typename L>
    simdjson_really_inline simd8<L> lookup_16(
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
    simdjson_really_inline simd8() : base8_numeric<int8_t>() {}
    simdjson_really_inline simd8(const __m512i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const int8_t values[64]) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15,
      int8_t v16, int8_t v17, int8_t v18, int8_t v19, int8_t v20, int8_t v21, int8_t v22, int8_t v23,
      int8_t v24, int8_t v25, int8_t v26, int8_t v27, int8_t v28, int8_t v29, int8_t v30, int8_t v31,
      int8_t v32, int8_t v33, int8_t v34, int8_t v35, int8_t v36, int8_t v37, int8_t v38, int8_t v39,
      int8_t v40, int8_t v41, int8_t v42, int8_t v43, int8_t v44, int8_t v45, int8_t v46, int8_t v47,
      int8_t v48, int8_t v49, int8_t v50, int8_t v51, int8_t v52, int8_t v53, int8_t v54, int8_t v55,
      int8_t v56, int8_t v57, int8_t v58, int8_t v59, int8_t v60, int8_t v61, int8_t v62, int8_t v63
    ) : simd8(_mm512_set_epi8(
      v63, v62, v61, v60, v59, v58, v57, v56,
      v55, v54, v53, v52, v51, v50, v49, v48,
      v47, v46, v45, v44, v43, v42, v41, v40,
      v39, v38, v37, v36, v35, v34, v33, v32,
      v31, v30, v29, v28, v27, v26, v25, v24,
      v23, v22, v21, v20, v19, v18, v17, v16,
      v15, v14, v13, v12, v11, v10,  v9,  v8,
       v7,  v6,  v5,  v4,  v3,  v2,  v1,  v0
    )) {}

    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Order-sensitive comparisons
    simdjson_really_inline simd8<int8_t> max_val(const simd8<int8_t> other) const { return _mm512_max_epi8(*this, other); }
    simdjson_really_inline simd8<int8_t> min_val(const simd8<int8_t> other) const { return _mm512_min_epi8(*this, other); }

    simdjson_really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm512_maskz_abs_epi8(_mm512_cmpgt_epi8_mask(*this, other),_mm512_set1_epi8(uint8_t(0x80))); }
    simdjson_really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return _mm512_maskz_abs_epi8(_mm512_cmpgt_epi8_mask(other, *this),_mm512_set1_epi8(uint8_t(0x80))); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    simdjson_really_inline simd8() : base8_numeric<uint8_t>() {}
    simdjson_really_inline simd8(const __m512i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    simdjson_really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_really_inline simd8(const uint8_t values[64]) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15,
      uint8_t v16, uint8_t v17, uint8_t v18, uint8_t v19, uint8_t v20, uint8_t v21, uint8_t v22, uint8_t v23,
      uint8_t v24, uint8_t v25, uint8_t v26, uint8_t v27, uint8_t v28, uint8_t v29, uint8_t v30, uint8_t v31,
      uint8_t v32, uint8_t v33, uint8_t v34, uint8_t v35, uint8_t v36, uint8_t v37, uint8_t v38, uint8_t v39,
      uint8_t v40, uint8_t v41, uint8_t v42, uint8_t v43, uint8_t v44, uint8_t v45, uint8_t v46, uint8_t v47,
      uint8_t v48, uint8_t v49, uint8_t v50, uint8_t v51, uint8_t v52, uint8_t v53, uint8_t v54, uint8_t v55,
      uint8_t v56, uint8_t v57, uint8_t v58, uint8_t v59, uint8_t v60, uint8_t v61, uint8_t v62, uint8_t v63
    ) : simd8(_mm512_set_epi8(
      v63, v62, v61, v60, v59, v58, v57, v56,
      v55, v54, v53, v52, v51, v50, v49, v48,
      v47, v46, v45, v44, v43, v42, v41, v40,
      v39, v38, v37, v36, v35, v34, v33, v32,
      v31, v30, v29, v28, v27, v26, v25, v24,
      v23, v22, v21, v20, v19, v18, v17, v16,
      v15, v14, v13, v12, v11, v10,  v9,  v8,
       v7,  v6,  v5,  v4,  v3,  v2,  v1,  v0
    )) {}

    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Saturated math
    simdjson_really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm512_adds_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm512_subs_epu8(*this, other); }

    // Order-specific operations
    simdjson_really_inline simd8<uint8_t> max_val(const simd8<uint8_t> other) const { return _mm512_max_epu8(*this, other); }
    simdjson_really_inline simd8<uint8_t> min_val(const simd8<uint8_t> other) const { return _mm512_min_epu8(other, *this); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    simdjson_really_inline uint64_t operator<=(const simd8<uint8_t> other) const { return other.max_val(*this) == other; }
    simdjson_really_inline uint64_t operator>=(const simd8<uint8_t> other) const { return other.min_val(*this) == other; }
    simdjson_really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    simdjson_really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->lt_bits(other).any_bits_set(); }

    // Bit-specific operations
    simdjson_really_inline simd8<bool> bits_not_set() const { return _mm512_mask_blend_epi8(*this == uint8_t(0), _mm512_set1_epi8(0), _mm512_set1_epi8(-1)); }
    simdjson_really_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    simdjson_really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }

    simdjson_really_inline bool is_ascii() const { return _mm512_movepi8_mask(*this) == 0; }
    simdjson_really_inline bool bits_not_set_anywhere() const {
      return !_mm512_test_epi8_mask(*this, *this);
    }
    simdjson_really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    simdjson_really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return !_mm512_test_epi8_mask(*this, bits); }
    simdjson_really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm512_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    simdjson_really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm512_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    simdjson_really_inline uint64_t get_bit() const { return _mm512_movepi8_mask(_mm512_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 1, "Icelake kernel should use one register per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T>& other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

    simdjson_really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1) : chunks{chunk0, chunk1} {}
    simdjson_really_inline simd8x64(const simd8<T> chunk0) : chunks{chunk0} {}
    simdjson_really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr)} {}

    simdjson_really_inline uint64_t compress(uint64_t mask, T * output) const {
      this->chunks[0].compress(mask, output);
      return 64 - count_ones(mask);
    }

    simdjson_really_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
    }

    simdjson_really_inline simd8<T> reduce_or() const {
      return this->chunks[0];
    }

    simdjson_really_inline simd8x64<T> bit_or(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return simd8x64<T>(
        this->chunks[0] | mask
      );
    }

    simdjson_really_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->chunks[0] == mask;
    }

    simdjson_really_inline uint64_t eq(const simd8x64<uint8_t> &other) const {
      return this->chunks[0] == other.chunks[0];
    }

    simdjson_really_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return this->chunks[0] <= mask;
    }
  }; // struct simd8x64<T>

} // namespace simd

} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_ICELAKE_SIMD_H
