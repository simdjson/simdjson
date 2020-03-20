#ifndef SIMDJSON_ARM64_SIMD_H
#define SIMDJSON_ARM64_SIMD_H

#include "simdjson.h"
#include "simdprune_tables.h"
#include "arm64/bitmanipulation.h"
#include "arm64/intrinsics.h"

namespace simdjson::arm64::simd {

  template<typename T>
  struct simd8;

  //
  // Base class of simd8<uint8_t> and simd8<bool>, both of which use uint8x16_t internally.
  //
  template<typename T, typename Mask=simd8<bool>>
  struct base_u8 {
    uint8x16_t value;
    static const int SIZE = sizeof(value);

    // Conversion from/to SIMD register
    really_inline base_u8(const uint8x16_t _value) : value(_value) {}
    really_inline operator const uint8x16_t&() const { return this->value; }
    really_inline operator uint8x16_t&() { return this->value; }

    // Bit operations
    really_inline simd8<T> operator|(const simd8<T> other) const { return vorrq_u8(*this, other); }
    really_inline simd8<T> operator&(const simd8<T> other) const { return vandq_u8(*this, other); }
    really_inline simd8<T> operator^(const simd8<T> other) const { return veorq_u8(*this, other); }
    really_inline simd8<T> bit_andnot(const simd8<T> other) const { return vbicq_u8(*this, other); }
    really_inline simd8<T> operator~() const { return *this ^ 0xFFu; }
    really_inline simd8<T>& operator|=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast | other; return *this_cast; }
    really_inline simd8<T>& operator&=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast & other; return *this_cast; }
    really_inline simd8<T>& operator^=(const simd8<T> other) { auto this_cast = (simd8<T>*)this; *this_cast = *this_cast ^ other; return *this_cast; }

    really_inline Mask operator==(const simd8<T> other) const { return vceqq_u8(*this, other); }

    template<int N=1>
    really_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return vextq_u8(prev_chunk, *this, 16 - N);
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base_u8<bool> {
    typedef uint16_t bitmask_t;
    typedef uint32_t bitmask2_t;

    static really_inline simd8<bool> splat(bool _value) { return vmovq_n_u8(-(!!_value)); }

    really_inline simd8(const uint8x16_t _value) : base_u8<bool>(_value) {}
    // False constructor
    really_inline simd8() : simd8(vdupq_n_u8(0)) {}
    // Splat constructor
    really_inline simd8(bool _value) : simd8(splat(_value)) {}

    // We return uint32_t instead of uint16_t because that seems to be more efficient for most
    // purposes (cutting it down to uint16_t costs performance in some compilers).
    really_inline uint32_t to_bitmask() const {
      const uint8x16_t bit_mask = {0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                   0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
      auto minput = *this & bit_mask;
      uint8x16_t tmp = vpaddq_u8(minput, minput);
      tmp = vpaddq_u8(tmp, tmp);
      tmp = vpaddq_u8(tmp, tmp);
      return vgetq_lane_u16(vreinterpretq_u16_u8(tmp), 0);
    }
    really_inline bool any() const { return vmaxvq_u8(*this) != 0; }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base_u8<uint8_t> {
    static really_inline uint8x16_t splat(uint8_t _value) { return vmovq_n_u8(_value); }
    static really_inline uint8x16_t zero() { return vdupq_n_u8(0); }
    static really_inline uint8x16_t load(const uint8_t* values) { return vld1q_u8(values); }

    really_inline simd8(const uint8x16_t _value) : base_u8<uint8_t>(_value) {}
    // Zero constructor
    really_inline simd8() : simd8(zero()) {}
    // Array constructor
    really_inline simd8(const uint8_t values[16]) : simd8(load(values)) {}
    // Splat constructor
    really_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Member-by-member initialization
    really_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(uint8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
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

    // Store to array
    really_inline void store(uint8_t dst[16]) const { return vst1q_u8(dst, *this); }

    // Saturated math
    really_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return vqaddq_u8(*this, other); }
    really_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return vqsubq_u8(*this, other); }

    // Addition/subtraction are the same for signed and unsigned
    really_inline simd8<uint8_t> operator+(const simd8<uint8_t> other) const { return vaddq_u8(*this, other); }
    really_inline simd8<uint8_t> operator-(const simd8<uint8_t> other) const { return vsubq_u8(*this, other); }
    really_inline simd8<uint8_t>& operator+=(const simd8<uint8_t> other) { *this = *this + other; return *this; }
    really_inline simd8<uint8_t>& operator-=(const simd8<uint8_t> other) { *this = *this - other; return *this; }

    // Order-specific operations
    really_inline uint8_t max() const { return vmaxvq_u8(*this); }
    really_inline uint8_t min() const { return vminvq_u8(*this); }
    really_inline simd8<uint8_t> max(const simd8<uint8_t> other) const { return vmaxq_u8(*this, other); }
    really_inline simd8<uint8_t> min(const simd8<uint8_t> other) const { return vminq_u8(*this, other); }
    really_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return vcleq_u8(*this, other); }
    really_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return vcgeq_u8(*this, other); }
    really_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return vcltq_u8(*this, other); }
    really_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return vcgtq_u8(*this, other); }
    // Same as >, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    really_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this > other); }
    // Same as <, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero. For ARM, returns all 1's.
    really_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this < other); }

    // Bit-specific operations
    really_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return vtstq_u8(*this, bits); }
    really_inline bool any_bits_set_anywhere() const { return this->max() != 0; }
    really_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return (*this & bits).any_bits_set_anywhere(); }
    template<int N>
    really_inline simd8<uint8_t> shr() const { return vshrq_n_u8(*this, N); }
    template<int N>
    really_inline simd8<uint8_t> shl() const { return vshlq_n_u8(*this, N); }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
    }


    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 16 - count_ones(mask) bytes of the result are significant but 16 bytes
    // get written.
    // Design consideration: it seems like a function with the
    // signature simd8<L> compress(uint16_t mask) would be
    // sensible, but the AVX ISA makes this kind of approach difficult.
    template<typename L>
    really_inline void compress(uint16_t mask, L * output) const {
      // this particular implementation was inspired by work done by @animetosho
      // we do it in two steps, first 8 bytes and then second 8 bytes
      uint8_t mask1 = static_cast<uint8_t>(mask); // least significant 8 bits
      uint8_t mask2 = static_cast<uint8_t>(mask >> 8); // most significant 8 bits
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register, using only
      // two instructions on most compilers.
      uint64x2_t shufmask64 = {thintable_epi8[mask1], thintable_epi8[mask2]};
      uint8x16_t shufmask = vreinterpretq_u8_u64(shufmask64);
      // we increment by 0x08 the second half of the mask
      uint8x16_t inc = {0, 0, 0, 0, 0, 0, 0, 0, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08};
      shufmask = vaddq_u8(shufmask, inc);
      // this is the version "nearly pruned"
      uint8x16_t pruned = vqtbl1q_u8(*this, shufmask);
      // we still need to put the two halves together.
      // we compute the popcount of the first half:
      int pop1 = BitsSetTable256mul2[mask1];
      // then load the corresponding mask, what it does is to write
      // only the first pop1 bytes from the first 8 bytes, and then
      // it fills in with the bytes from the second 8 bytes + some filling
      // at the end.
      uint8x16_t compactmask = vld1q_u8((const uint8_t *)(pshufb_combine_table + pop1 * 8));
      uint8x16_t answer = vqtbl1q_u8(pruned, compactmask);
      vst1q_u8((uint8_t*) output, answer);
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

    template<typename T>
    really_inline simd8<uint8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_u8(*this, simd8<uint8_t>(original));
    }
  };

  // Signed bytes
  template<>
  struct simd8<int8_t> {
    int8x16_t value;

    static really_inline simd8<int8_t> splat(int8_t _value) { return vmovq_n_s8(_value); }
    static really_inline simd8<int8_t> zero() { return vdupq_n_s8(0); }
    static really_inline simd8<int8_t> load(const int8_t values[16]) { return vld1q_s8(values); }

    // Conversion from/to SIMD register
    really_inline simd8(const int8x16_t _value) : value{_value} {}
    really_inline operator const int8x16_t&() const { return this->value; }
    really_inline operator int8x16_t&() { return this->value; }

    // Zero constructor
    really_inline simd8() : simd8(zero()) {}
    // Splat constructor
    really_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    really_inline simd8(const int8_t* values) : simd8(load(values)) {}
    // Member-by-member initialization
    really_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3, int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(int8x16_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}
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

    // Store to array
    really_inline void store(int8_t dst[16]) const { return vst1q_s8(dst, *this); }

    // Explicit conversion to/from unsigned
    really_inline explicit simd8(const uint8x16_t other): simd8(vreinterpretq_s8_u8(other)) {}
    really_inline explicit operator simd8<uint8_t>() const { return vreinterpretq_u8_s8(*this); }

    // Math
    really_inline simd8<int8_t> operator+(const simd8<int8_t> other) const { return vaddq_s8(*this, other); }
    really_inline simd8<int8_t> operator-(const simd8<int8_t> other) const { return vsubq_s8(*this, other); }
    really_inline simd8<int8_t>& operator+=(const simd8<int8_t> other) { *this = *this + other; return *this; }
    really_inline simd8<int8_t>& operator-=(const simd8<int8_t> other) { *this = *this - other; return *this; }

    // Order-sensitive comparisons
    really_inline simd8<int8_t> max(const simd8<int8_t> other) const { return vmaxq_s8(*this, other); }
    really_inline simd8<int8_t> min(const simd8<int8_t> other) const { return vminq_s8(*this, other); }
    really_inline simd8<bool> operator>(const simd8<int8_t> other) const { return vcgtq_s8(*this, other); }
    really_inline simd8<bool> operator<(const simd8<int8_t> other) const { return vcltq_s8(*this, other); }
    really_inline simd8<bool> operator==(const simd8<int8_t> other) const { return vceqq_s8(*this, other); }

    template<int N=1>
    really_inline simd8<int8_t> prev(const simd8<int8_t> prev_chunk) const {
      return vextq_s8(prev_chunk, *this, 16 - N);
    }

    // Perform a lookup assuming no value is larger than 16
    template<typename L>
    really_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return lookup_table.apply_lookup_16_to(*this);
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

    template<typename T>
    really_inline simd8<int8_t> apply_lookup_16_to(const simd8<T> original) {
      return vqtbl1q_s8(*this, simd8<uint8_t>(original));
    }
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

    really_inline void compress(uint64_t mask, T * output) const {
      this->chunks[0].compress(mask, output);
      this->chunks[1].compress(mask >> 16, output + 16 - count_ones(mask & 0xFFFF));
      this->chunks[2].compress(mask >> 32, output + 32 - count_ones(mask & 0xFFFFFFFF));
      this->chunks[3].compress(mask >> 48, output + 48 - count_ones(mask & 0xFFFFFFFFFFFF));
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

    template <typename R=bool, typename F>
    really_inline simd8x64<R> map(F const& map_chunk) const {
      return simd8x64<R>(
        map_chunk(this->chunks[0]),
        map_chunk(this->chunks[1]),
        map_chunk(this->chunks[2]),
        map_chunk(this->chunks[3])
      );
    }

    template <typename R=bool, typename F>
    really_inline simd8x64<R> map(const simd8x64<T> b, F const& map_chunk) const {
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
      const uint8x16_t bit_mask = {
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80
      };
      // Add each of the elements next to each other, successively, to stuff each 8 byte mask into one.
      uint8x16_t sum0 = vpaddq_u8(this->chunks[0] & bit_mask, this->chunks[1] & bit_mask);
      uint8x16_t sum1 = vpaddq_u8(this->chunks[2] & bit_mask, this->chunks[3] & bit_mask);
      sum0 = vpaddq_u8(sum0, sum1);
      sum0 = vpaddq_u8(sum0, sum0);
      return vgetq_lane_u64(vreinterpretq_u64_u8(sum0), 0);
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

} // namespace simdjson::arm64::simd

#endif // SIMDJSON_ARM64_SIMD_H
