#ifndef SIMDJSON_PPC64_SIMD_H
#define SIMDJSON_PPC64_SIMD_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/ppc64/base.h"
#include "simdjson/ppc64/bitmanipulation.h"
#include "simdjson/internal/simdprune_tables.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <type_traits>

namespace simdjson {
namespace ppc64 {
namespace {
namespace simd {

using __m128i = __vector unsigned char;

template <typename Child> struct base {
  __m128i value;

  // Zero constructor
  simdjson_inline base() : value{__m128i()} {}

  // Conversion from SIMD register
  simdjson_inline base(const __m128i _value) : value(_value) {}

  // Conversion to SIMD register
  simdjson_inline operator const __m128i &() const {
    return this->value;
  }
  simdjson_inline operator __m128i &() { return this->value; }

  // Bit operations
  simdjson_inline Child operator|(const Child other) const {
    return vec_or(this->value, (__m128i)other);
  }
  simdjson_inline Child operator&(const Child other) const {
    return vec_and(this->value, (__m128i)other);
  }
  simdjson_inline Child operator^(const Child other) const {
    return vec_xor(this->value, (__m128i)other);
  }
  simdjson_inline Child bit_andnot(const Child other) const {
    return vec_andc(this->value, (__m128i)other);
  }
  simdjson_inline Child &operator|=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast | other;
    return *this_cast;
  }
  simdjson_inline Child &operator&=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast & other;
    return *this_cast;
  }
  simdjson_inline Child &operator^=(const Child other) {
    auto this_cast = static_cast<Child*>(this);
    *this_cast = *this_cast ^ other;
    return *this_cast;
  }
};

template <typename T, typename Mask = simd8<bool>>
struct base8 : base<simd8<T>> {
  typedef uint16_t bitmask_t;
  typedef uint32_t bitmask2_t;

  simdjson_inline base8() : base<simd8<T>>() {}
  simdjson_inline base8(const __m128i _value) : base<simd8<T>>(_value) {}

  friend simdjson_inline Mask operator==(const simd8<T> lhs, const simd8<T> rhs) {
    return (__m128i)vec_cmpeq(lhs.value, (__m128i)rhs);
  }

  static const int SIZE = sizeof(base<simd8<T>>::value);

  template <int N = 1>
  simdjson_inline simd8<T> prev(simd8<T> prev_chunk) const {
    __m128i chunk = this->value;
#ifdef __LITTLE_ENDIAN__
    chunk = (__m128i)vec_reve(this->value);
    prev_chunk = (__m128i)vec_reve((__m128i)prev_chunk);
#endif
    chunk = (__m128i)vec_sld((__m128i)prev_chunk, (__m128i)chunk, 16 - N);
#ifdef __LITTLE_ENDIAN__
    chunk = (__m128i)vec_reve((__m128i)chunk);
#endif
    return chunk;
  }
};

// SIMD byte mask type (returned by things like eq and gt)
template <> struct simd8<bool> : base8<bool> {
  static simdjson_inline simd8<bool> splat(bool _value) {
    return (__m128i)vec_splats((unsigned char)(-(!!_value)));
  }

  simdjson_inline simd8<bool>() : base8<bool>() {}
  simdjson_inline simd8<bool>(const __m128i _value)
      : base8<bool>(_value) {}
  // Splat constructor
  simdjson_inline simd8<bool>(bool _value)
      : base8<bool>(splat(_value)) {}

  simdjson_inline int to_bitmask() const {
    __vector unsigned long long result;
    const __m128i perm_mask = {0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x40,
                               0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 0x08, 0x00};

    result = ((__vector unsigned long long)vec_vbpermq((__m128i)this->value,
                                                       (__m128i)perm_mask));
#ifdef __LITTLE_ENDIAN__
    return static_cast<int>(result[1]);
#else
    return static_cast<int>(result[0]);
#endif
  }
  simdjson_inline bool any() const {
    return !vec_all_eq(this->value, (__m128i)vec_splats(0));
  }
  simdjson_inline simd8<bool> operator~() const {
    return this->value ^ (__m128i)splat(true);
  }
};

template <typename T> struct base8_numeric : base8<T> {
  static simdjson_inline simd8<T> splat(T value) {
    (void)value;
    return (__m128i)vec_splats(value);
  }
  static simdjson_inline simd8<T> zero() { return splat(0); }
  static simdjson_inline simd8<T> load(const T values[16]) {
    return (__m128i)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(values)));
  }
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  static simdjson_inline simd8<T> repeat_16(T v0, T v1, T v2, T v3, T v4,
                                                   T v5, T v6, T v7, T v8, T v9,
                                                   T v10, T v11, T v12, T v13,
                                                   T v14, T v15) {
    return simd8<T>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13,
                    v14, v15);
  }

  simdjson_inline base8_numeric() : base8<T>() {}
  simdjson_inline base8_numeric(const __m128i _value)
      : base8<T>(_value) {}

  // Store to array
  simdjson_inline void store(T dst[16]) const {
    vec_vsx_st(this->value, 0, reinterpret_cast<__m128i *>(dst));
  }

  // Override to distinguish from bool version
  simdjson_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

  // Addition/subtraction are the same for signed and unsigned
  simdjson_inline simd8<T> operator+(const simd8<T> other) const {
    return (__m128i)((__m128i)this->value + (__m128i)other);
  }
  simdjson_inline simd8<T> operator-(const simd8<T> other) const {
    return (__m128i)((__m128i)this->value - (__m128i)other);
  }
  simdjson_inline simd8<T> &operator+=(const simd8<T> other) {
    *this = *this + other;
    return *static_cast<simd8<T> *>(this);
  }
  simdjson_inline simd8<T> &operator-=(const simd8<T> other) {
    *this = *this - other;
    return *static_cast<simd8<T> *>(this);
  }

  // Perform a lookup assuming the value is between 0 and 16 (undefined behavior
  // for out of range values)
  template <typename L>
  simdjson_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
    return (__m128i)vec_perm((__m128i)lookup_table, (__m128i)lookup_table, this->value);
  }

  // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted
  // as a bitset). Passing a 0 value for mask would be equivalent to writing out
  // every byte to output. Only the first 16 - count_ones(mask) bytes of the
  // result are significant but 16 bytes get written. Design consideration: it
  // seems like a function with the signature simd8<L> compress(uint32_t mask)
  // would be sensible, but the AVX ISA makes this kind of approach difficult.
  template <typename L>
  simdjson_inline void compress(uint16_t mask, L *output) const {
    using internal::BitsSetTable256mul2;
    using internal::pshufb_combine_table;
    using internal::thintable_epi8;
    // this particular implementation was inspired by work done by @animetosho
    // we do it in two steps, first 8 bytes and then second 8 bytes
    uint8_t mask1 = uint8_t(mask);      // least significant 8 bits
    uint8_t mask2 = uint8_t(mask >> 8); // most significant 8 bits
    // next line just loads the 64-bit values thintable_epi8[mask1] and
    // thintable_epi8[mask2] into a 128-bit register, using only
    // two instructions on most compilers.
#ifdef __LITTLE_ENDIAN__
    __m128i shufmask = (__m128i)(__vector unsigned long long){
        thintable_epi8[mask1], thintable_epi8[mask2]};
#else
    __m128i shufmask = (__m128i)(__vector unsigned long long){
        thintable_epi8[mask2], thintable_epi8[mask1]};
    shufmask = (__m128i)vec_reve((__m128i)shufmask);
#endif
    // we increment by 0x08 the second half of the mask
    shufmask = ((__m128i)shufmask) +
               ((__m128i)(__vector int){0, 0, 0x08080808, 0x08080808});

    // this is the version "nearly pruned"
    __m128i pruned = vec_perm(this->value, this->value, shufmask);
    // we still need to put the two halves together.
    // we compute the popcount of the first half:
    int pop1 = BitsSetTable256mul2[mask1];
    // then load the corresponding mask, what it does is to write
    // only the first pop1 bytes from the first 8 bytes, and then
    // it fills in with the bytes from the second 8 bytes + some filling
    // at the end.
    __m128i compactmask =
        vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(pshufb_combine_table + pop1 * 8));
    __m128i answer = vec_perm(pruned, (__m128i)vec_splats(0), compactmask);
    vec_vsx_st(answer, 0, reinterpret_cast<__m128i *>(output));
  }

  template <typename L>
  simdjson_inline simd8<L>
  lookup_16(L replace0, L replace1, L replace2, L replace3, L replace4,
            L replace5, L replace6, L replace7, L replace8, L replace9,
            L replace10, L replace11, L replace12, L replace13, L replace14,
            L replace15) const {
    return lookup_16(simd8<L>::repeat_16(
        replace0, replace1, replace2, replace3, replace4, replace5, replace6,
        replace7, replace8, replace9, replace10, replace11, replace12,
        replace13, replace14, replace15));
  }
};

// Signed bytes
template <> struct simd8<int8_t> : base8_numeric<int8_t> {
  simdjson_inline simd8() : base8_numeric<int8_t>() {}
  simdjson_inline simd8(const __m128i _value)
      : base8_numeric<int8_t>(_value) {}
  // Splat constructor
  simdjson_inline simd8(int8_t _value) : simd8(splat(_value)) {}
  // Array constructor
  simdjson_inline simd8(const int8_t *values) : simd8(load(values)) {}
  // Member-by-member initialization
  simdjson_inline simd8(int8_t v0, int8_t v1, int8_t v2, int8_t v3,
                               int8_t v4, int8_t v5, int8_t v6, int8_t v7,
                               int8_t v8, int8_t v9, int8_t v10, int8_t v11,
                               int8_t v12, int8_t v13, int8_t v14, int8_t v15)
      : simd8((__m128i)(__vector signed char){v0, v1, v2, v3, v4, v5, v6, v7,
                                              v8, v9, v10, v11, v12, v13, v14,
                                              v15}) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  simdjson_inline static simd8<int8_t>
  repeat_16(int8_t v0, int8_t v1, int8_t v2, int8_t v3, int8_t v4, int8_t v5,
            int8_t v6, int8_t v7, int8_t v8, int8_t v9, int8_t v10, int8_t v11,
            int8_t v12, int8_t v13, int8_t v14, int8_t v15) {
    return simd8<int8_t>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                         v13, v14, v15);
  }

  // Order-sensitive comparisons
  simdjson_inline simd8<int8_t>
  max_val(const simd8<int8_t> other) const {
    return (__m128i)vec_max((__vector signed char)this->value,
                            (__vector signed char)(__m128i)other);
  }
  simdjson_inline simd8<int8_t>
  min_val(const simd8<int8_t> other) const {
    return (__m128i)vec_min((__vector signed char)this->value,
                            (__vector signed char)(__m128i)other);
  }
  simdjson_inline simd8<bool>
  operator>(const simd8<int8_t> other) const {
    return (__m128i)vec_cmpgt((__vector signed char)this->value,
                              (__vector signed char)(__m128i)other);
  }
  simdjson_inline simd8<bool>
  operator<(const simd8<int8_t> other) const {
    return (__m128i)vec_cmplt((__vector signed char)this->value,
                              (__vector signed char)(__m128i)other);
  }
};

// Unsigned bytes
template <> struct simd8<uint8_t> : base8_numeric<uint8_t> {
  simdjson_inline simd8() : base8_numeric<uint8_t>() {}
  simdjson_inline simd8(const __m128i _value)
      : base8_numeric<uint8_t>(_value) {}
  // Splat constructor
  simdjson_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
  // Array constructor
  simdjson_inline simd8(const uint8_t *values) : simd8(load(values)) {}
  // Member-by-member initialization
  simdjson_inline
  simd8(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4, uint8_t v5,
        uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9, uint8_t v10,
        uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15)
      : simd8((__m128i){v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                        v13, v14, v15}) {}
  // Repeat 16 values as many times as necessary (usually for lookup tables)
  simdjson_inline static simd8<uint8_t>
  repeat_16(uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3, uint8_t v4,
            uint8_t v5, uint8_t v6, uint8_t v7, uint8_t v8, uint8_t v9,
            uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14,
            uint8_t v15) {
    return simd8<uint8_t>(v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
                          v13, v14, v15);
  }

  // Saturated math
  simdjson_inline simd8<uint8_t>
  saturating_add(const simd8<uint8_t> other) const {
    return (__m128i)vec_adds(this->value, (__m128i)other);
  }
  simdjson_inline simd8<uint8_t>
  saturating_sub(const simd8<uint8_t> other) const {
    return (__m128i)vec_subs(this->value, (__m128i)other);
  }

  // Order-specific operations
  simdjson_inline simd8<uint8_t>
  max_val(const simd8<uint8_t> other) const {
    return (__m128i)vec_max(this->value, (__m128i)other);
  }
  simdjson_inline simd8<uint8_t>
  min_val(const simd8<uint8_t> other) const {
    return (__m128i)vec_min(this->value, (__m128i)other);
  }
  // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
  simdjson_inline simd8<uint8_t>
  gt_bits(const simd8<uint8_t> other) const {
    return this->saturating_sub(other);
  }
  // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
  simdjson_inline simd8<uint8_t>
  lt_bits(const simd8<uint8_t> other) const {
    return other.saturating_sub(*this);
  }
  simdjson_inline simd8<bool>
  operator<=(const simd8<uint8_t> other) const {
    return other.max_val(*this) == other;
  }
  simdjson_inline simd8<bool>
  operator>=(const simd8<uint8_t> other) const {
    return other.min_val(*this) == other;
  }
  simdjson_inline simd8<bool>
  operator>(const simd8<uint8_t> other) const {
    return this->gt_bits(other).any_bits_set();
  }
  simdjson_inline simd8<bool>
  operator<(const simd8<uint8_t> other) const {
    return this->gt_bits(other).any_bits_set();
  }

  // Bit-specific operations
  simdjson_inline simd8<bool> bits_not_set() const {
    return (__m128i)vec_cmpeq(this->value, (__m128i)vec_splats(uint8_t(0)));
  }
  simdjson_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const {
    return (*this & bits).bits_not_set();
  }
  simdjson_inline simd8<bool> any_bits_set() const {
    return ~this->bits_not_set();
  }
  simdjson_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const {
    return ~this->bits_not_set(bits);
  }
  simdjson_inline bool bits_not_set_anywhere() const {
    return vec_all_eq(this->value, (__m128i)vec_splats(0));
  }
  simdjson_inline bool any_bits_set_anywhere() const {
    return !bits_not_set_anywhere();
  }
  simdjson_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const {
    return vec_all_eq(vec_and(this->value, (__m128i)bits),
                      (__m128i)vec_splats(0));
  }
  simdjson_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const {
    return !bits_not_set_anywhere(bits);
  }
  template <int N> simdjson_inline simd8<uint8_t> shr() const {
    return simd8<uint8_t>(
        (__m128i)vec_sr(this->value, (__m128i)vec_splat_u8(N)));
  }
  template <int N> simdjson_inline simd8<uint8_t> shl() const {
    return simd8<uint8_t>(
        (__m128i)vec_sl(this->value, (__m128i)vec_splat_u8(N)));
  }
};

template <typename T> struct simd8x64 {
  static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
  static_assert(NUM_CHUNKS == 4,
                "PPC64 kernel should use four registers per 64-byte block.");
  const simd8<T> chunks[NUM_CHUNKS];

  simd8x64(const simd8x64<T> &o) = delete; // no copy allowed
  simd8x64<T> &
  operator=(const simd8<T>& other) = delete; // no assignment allowed
  simd8x64() = delete;                      // no default constructor allowed

  simdjson_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1,
                                  const simd8<T> chunk2, const simd8<T> chunk3)
      : chunks{chunk0, chunk1, chunk2, chunk3} {}
  simdjson_inline simd8x64(const T ptr[64])
      : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr + 16),
               simd8<T>::load(ptr + 32), simd8<T>::load(ptr + 48)} {}

  simdjson_inline void store(T ptr[64]) const {
    this->chunks[0].store(ptr + sizeof(simd8<T>) * 0);
    this->chunks[1].store(ptr + sizeof(simd8<T>) * 1);
    this->chunks[2].store(ptr + sizeof(simd8<T>) * 2);
    this->chunks[3].store(ptr + sizeof(simd8<T>) * 3);
  }

  simdjson_inline simd8<T> reduce_or() const {
    return (this->chunks[0] | this->chunks[1]) |
           (this->chunks[2] | this->chunks[3]);
  }

  simdjson_inline uint64_t compress(uint64_t mask, T *output) const {
    this->chunks[0].compress(uint16_t(mask), output);
    this->chunks[1].compress(uint16_t(mask >> 16),
                             output + 16 - count_ones(mask & 0xFFFF));
    this->chunks[2].compress(uint16_t(mask >> 32),
                             output + 32 - count_ones(mask & 0xFFFFFFFF));
    this->chunks[3].compress(uint16_t(mask >> 48),
                             output + 48 - count_ones(mask & 0xFFFFFFFFFFFF));
    return 64 - count_ones(mask);
  }

  simdjson_inline uint64_t to_bitmask() const {
    uint64_t r0 = uint32_t(this->chunks[0].to_bitmask());
    uint64_t r1 = this->chunks[1].to_bitmask();
    uint64_t r2 = this->chunks[2].to_bitmask();
    uint64_t r3 = this->chunks[3].to_bitmask();
    return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
  }

  simdjson_inline uint64_t eq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] == mask, this->chunks[1] == mask,
                          this->chunks[2] == mask, this->chunks[3] == mask)
        .to_bitmask();
  }

  simdjson_inline uint64_t eq(const simd8x64<uint8_t> &other) const {
    return simd8x64<bool>(this->chunks[0] == other.chunks[0],
                          this->chunks[1] == other.chunks[1],
                          this->chunks[2] == other.chunks[2],
                          this->chunks[3] == other.chunks[3])
        .to_bitmask();
  }

  simdjson_inline uint64_t lteq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] <= mask, this->chunks[1] <= mask,
                          this->chunks[2] <= mask, this->chunks[3] <= mask)
        .to_bitmask();
  }
}; // struct simd8x64<T>

} // namespace simd
} // unnamed namespace
} // namespace ppc64
} // namespace simdjson

#endif // SIMDJSON_PPC64_SIMD_INPUT_H
