#ifndef SIMDJSON_HASWELL_SIMD_H
#define SIMDJSON_HASWELL_SIMD_H

#include "simdjson/portability.h"

#ifdef IS_X86_64

#include "simdjson/common_defs.h"

TARGET_HASWELL
namespace simdjson::haswell::simd {

  // Forward-declared so they can be used by splat and friends.
  template<typename Child>
  struct base {
    __m256i value;

    // Zero constructor
    really_inline base() : value{__m256i()} {}

    // Conversion from SIMD register
    really_inline base(const __m256i _value) : value(_value) {}

    // Conversion to SIMD register
    really_inline operator const __m256i&() const { return this->value; }
    really_inline operator __m256i&() { return this->value; }

    // Bit operations
    really_inline Child operator|(const Child other) const { return _mm256_or_si256(*this, other); }
    really_inline Child operator&(const Child other) const { return _mm256_and_si256(*this, other); }
    really_inline Child operator^(const Child other) const { return _mm256_xor_si256(*this, other); }
    really_inline Child bit_andnot(const Child other) const { return _mm256_andnot_si256(*this, other); }
    really_inline Child operator~() const { return *this ^ 0xFFu; }
    really_inline Child& operator|=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast | other; return *this_cast; }
    really_inline Child& operator&=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast & other; return *this_cast; }
    really_inline Child& operator^=(const Child other) { auto this_cast = (Child*)this; *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    typedef uint32_t bitmask_t;
    typedef uint64_t bitmask2_t;

    really_inline base8() : base<simd8<T>>() {}
    really_inline base8(const __m256i _value) : base<simd8<T>>(_value) {}

    really_inline Mask operator==(const simd8<T> other) const { return _mm256_cmpeq_epi8(*this, other); }

    static const int SIZE = sizeof(base<T>::value);

    template<int N=1>
    really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return _mm256_alignr_epi8(*this, _mm256_permute2x128_si256(prev_chunk, *this, 0x21), 16 - N);
    }

    // TODO can we use alignr more directly, without a zero constant?
    template<int N=1>
    really_inline simd8<T> prev() const { return this->prev<N>(uint8_t(0)); }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static really_inline simd8<bool> splat(bool _value) { return _mm256_set1_epi8(-(!!_value)); }

    really_inline simd8<bool>() : base8() {}
    really_inline simd8<bool>(const __m256i _value) : base8<bool>(_value) {}
    // Splat constructor
    really_inline simd8<bool>(bool _value) : base8<bool>(splat(_value)) {}

    really_inline int to_bitmask() const { return _mm256_movemask_epi8(*this); }
    really_inline bool any() const { return !_mm256_testz_si256(*this, *this); }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static really_inline simd8<T> splat(T _value) { return _mm256_set1_epi8(_value); }
    static really_inline simd8<T> zero() { return _mm256_setzero_si256(); }
    static really_inline simd8<T> load(const T values[32]) {
      return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(values));
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static really_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    really_inline base8_numeric() : base8<T>() {}
    really_inline base8_numeric(const __m256i _value) : base8<T>(_value) {}

    // Store to array
    really_inline void store(T dst[32]) { return _mm256_storeu_si256(reinterpret_cast<__m256i *>(dst), *this); }

    // Addition/subtraction are the same for signed and unsigned
    really_inline simd8<T> operator+(const simd8<T> other) const { return _mm256_add_epi8(*this, other); }
    really_inline simd8<T> operator-(const simd8<T> other) const { return _mm256_sub_epi8(*this, other); }
    really_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *(simd8<T>*)this; }
    really_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *(simd8<T>*)this; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return _mm256_shuffle_epi8(lookup_table, *this);
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
    really_inline simd8(const __m256i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const int8_t values[32]) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15,
      int8_t v16, int8_t v17, int8_t v18, int8_t v19, int8_t v20, int8_t v21, int8_t v22, int8_t v23,
      int8_t v24, int8_t v25, int8_t v26, int8_t v27, int8_t v28, int8_t v29, int8_t v30, int8_t v31
    ) : simd8(_mm256_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15,
      v16,v17,v18,v19,v20,v21,v22,v23,
      v24,v25,v26,v27,v28,v29,v30,v31
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Order-sensitive comparisons
    really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return _mm256_max_epi8(*this, other); }
    really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return _mm256_min_epi8(*this, other); }
    really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return _mm256_cmpgt_epi8(*this, other); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    really_inline simd8() : base8_numeric<uint8_t>() {}
    really_inline simd8(const __m256i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const uint8_t values[32]) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15,
      uint8_t v16, uint8_t v17, uint8_t v18, uint8_t v19, uint8_t v20, uint8_t v21, uint8_t v22, uint8_t v23,
      uint8_t v24, uint8_t v25, uint8_t v26, uint8_t v27, uint8_t v28, uint8_t v29, uint8_t v30, uint8_t v31
    ) : simd8(_mm256_setr_epi8(
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15,
      v16,v17,v18,v19,v20,v21,v22,v23,
      v24,v25,v26,v27,v28,v29,v30,v31
    )) {}
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    really_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15,
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    // Saturated math
    really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return _mm256_adds_epu8(*this, other); }
    really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return _mm256_subs_epu8(*this, other); }

    // Order-specific operations
    really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return _mm256_max_epu8(*this, other); }
    really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return _mm256_min_epu8(*this, other); }
    really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max(*this) == other; }
    really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min(*this) == other; }
    really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->saturating_sub(other).any_bits_set(); }

    // Bit-specific operations
    really_inline simd8<bool> any_bits_set() const { return ~(*this == uint8_t(0)); }
    really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return (*this & bits).any_bits_set(); }
    really_inline bool bits_not_set_anywhere() const { return _mm256_testz_si256(*this, *this); }
    really_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    really_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const { return _mm256_testz_si256(*this, bits); }
    really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    really_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(_mm256_srli_epi16(*this, N)) & uint8_t(0xFFu >> N); }
    template<int N>
    really_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(_mm256_slli_epi16(*this, N)) & uint8_t(0xFFu << N); }
    // Get one of the bits and make a bitmask out of it.
    // e.g. value.get_bit<7>() gets the high bit
    template<int N>
    really_inline int get_bit() const { return _mm256_movemask_epi8(_mm256_slli_epi16(*this, 7-N)); }
  };

  template<typename T>
  struct simd8x64 {
    static const int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    const simd8<T> chunks[NUM_CHUNKS];

    really_inline simd8x64() : chunks{simd8<T>(), simd8<T>()} {}
    really_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1) : chunks{chunk0, chunk1} {}
    really_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+32)} {}

    template <typename F>
    static really_inline void each_index(F const& each) {
      each(0);
      each(1);
    }

    really_inline void store(T ptr[64]) {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
    }

    template <typename F>
    really_inline void each(F const& each_chunk) const
    {
      each_chunk(this->chunks[0]);
      each_chunk(this->chunks[1]);
    }

    template <typename R=bool, typename F>
    really_inline simd8x64<R> map(F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0]),
        map_chunk(this->chunks[1])
      );
    }

    template <typename R=bool, typename F>
    really_inline simd8x64<R> map(const simd8x64<uint8_t> b, F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0], b.chunks[0]),
        map_chunk(this->chunks[1], b.chunks[1])
      );
    }

    template <typename F>
    really_inline simd8<T> reduce(F const& reduce_pair) const {
      return reduce_pair(this->chunks[0], this->chunks[1]);
    }

    really_inline uint64_t to_bitmask() const {
      uint64_t r_lo = static_cast<uint32_t>(this->chunks[0].to_bitmask());
      uint64_t r_hi =                       this->chunks[1].to_bitmask();
      return r_lo | (r_hi << 32);
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

    really_inline simd8<uint8_t> last_chunk() const {
      return this->chunks[NUM_CHUNKS-1];
    }

  }; // struct simd8x64<T>

  // Get a comma separated list of the vector chunks
  #undef SIMD8X64_CHUNKS
  #define SIMD8X64_CHUNKS(vec) (vec.chunks[0]), (vec.chunks[1])

} // namespace simdjson::haswell::simd
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_HASWELL_SIMD_H
