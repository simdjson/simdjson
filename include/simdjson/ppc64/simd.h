#ifndef SIMDJSON_PPC64_SIMD_H
#define SIMDJSON_PPC64_SIMD_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/ppc64/base.h"
#include "simdjson/ppc64/bitmask.h"
#include "simdjson/internal/simdprune_tables.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <type_traits>

namespace simdjson {
namespace ppc64 {
namespace simd {

#if !(SIMDJSON_IS_PPC64 && SIMDJSON_IS_PPC64_VMX) && !defined(SIMDJSON_CONDITIONAL_INCLUDE)
  // Make errors a bit more manageable when editing on non-ARM
  struct __m128u { uint8_t buf[16]; };
  using __m128i = __m128u;
#else
using __m128u = __vector unsigned char;
using __m128i = __vector signed char;
#endif

template <typename Child> struct base {
  using simd_t = __m128u;
  simd_t value;

  // Zero constructor
  simdjson_inline base() : value{simd_t()} {}

  // Conversion from SIMD register
  simdjson_inline base(const simd_t _value) : value(_value) {}

  // Conversion to SIMD register
  simdjson_inline operator const simd_t &() const {
    return this->value;
  }
  simdjson_inline operator simd_t &() { return this->value; }

  // Bit operations
  simdjson_inline Child operator|(const Child other) const {
    return vec_or(this->value, (simd_t)other);
  }
  simdjson_inline Child operator&(const Child other) const {
    return vec_and(this->value, (simd_t)other);
  }
  simdjson_inline Child operator^(const Child other) const {
    return vec_xor(this->value, (simd_t)other);
  }
  simdjson_inline Child bit_andnot(const Child other) const {
    return vec_andc(this->value, (simd_t)other);
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
  using typename base<simd8<T>>::simd_t;
  static constexpr const int LANES = sizeof(simd_t);
  using bitmask_t = uint16_t;
  static_assert(sizeof(bitmask_t)*8 == LANES, "Bitmask type's bits must equal the simd type's bytes");

  simdjson_inline base8() : base<simd8<T>>() {}
  simdjson_inline base8(const simd_t _value) : base<simd8<T>>(_value) {}

  simdjson_inline Mask eq(const simd8<T> rhs) const { return (simd_t)vec_cmpeq(this->value, (simd_t)rhs); }
  friend simdjson_inline Mask operator==(const simd8<T> lhs, const simd8<T> rhs) { return lhs.eq(rhs); }

  template <int N = 1>
  simdjson_inline simd8<T> prev(const simd8<T>& prev_chunk) const {
    simd_t chunk = this->value;
#ifdef __LITTLE_ENDIAN__
    chunk = (simd_t)vec_reve(this->value);
    prev_chunk = (simd_t)vec_reve((simd_t)prev_chunk);
#endif
    chunk = (simd_t)vec_sld((simd_t)prev_chunk, (simd_t)chunk, 16 - N);
#ifdef __LITTLE_ENDIAN__
    chunk = (simd_t)vec_reve((simd_t)chunk);
#endif
    return chunk;
  }
};

// SIMD byte mask type (returned by things like eq and gt)
template <> struct simd8<bool> : base8<bool> {
  using typename base8<bool>::simd_t;

  static simdjson_inline simd8<bool> splat(bool _value) {
    return (simd_t)vec_splats((unsigned char)(-(!!_value)));
  }

  simdjson_inline simd8<bool>() : base8<bool>() {}
  simdjson_inline simd8<bool>(const simd_t _value)
      : base8<bool>(_value) {}
  // Splat constructor
  simdjson_inline simd8<bool>(bool _value)
      : base8<bool>(splat(_value)) {}

  simdjson_inline int to_bitmask() const {
    __vector unsigned long long result;
    const simd_t perm_mask = {0x78, 0x70, 0x68, 0x60, 0x58, 0x50, 0x48, 0x40,
                               0x38, 0x30, 0x28, 0x20, 0x18, 0x10, 0x08, 0x00};

    result = ((__vector unsigned long long)vec_vbpermq((simd_t)this->value,
                                                       (simd_t)perm_mask));
#ifdef __LITTLE_ENDIAN__
    return static_cast<int>(result[1]);
#else
    return static_cast<int>(result[0]);
#endif
  }
  simdjson_inline bool any() const {
    return !vec_all_eq(this->value, (simd_t)vec_splats(0));
  }
  simdjson_inline simd8<bool> operator~() const {
    return this->value ^ (simd_t)splat(true);
  }
};

template <typename T> struct base8_numeric : base8<T> {
  using typename base8<T>::simd_t;
  using base8<T>::LANES;

  static simdjson_inline simd8<T> splat(T value) {
    (void)value;
    return (simd_t)vec_splats(value);
  }
  static simdjson_inline simd8<T> zero() { return splat(0); }
  static simdjson_inline simd8<T> load(const T values[16]) {
    return (simd_t)(vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(values)));
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
  simdjson_inline base8_numeric(const simd_t _value)
      : base8<T>(_value) {}

  // Store to array
  simdjson_inline void store(T dst[16]) const {
    vec_vsx_st(this->value, 0, reinterpret_cast<simd_t *>(dst));
  }

  // Override to distinguish from bool version
  simdjson_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

  // Addition/subtraction are the same for signed and unsigned
  simdjson_inline simd8<T> operator+(const simd8<T> other) const {
    return (simd_t)((simd_t)this->value + (simd_t)other);
  }
  simdjson_inline simd8<T> operator-(const simd8<T> other) const {
    return (simd_t)((simd_t)this->value - (simd_t)other);
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
  simdjson_inline simd8<T> lookup_16(const simd8<T>& lookup_table) const {
    return (simd_t)vec_perm((simd_t)lookup_table, (simd_t)lookup_table, this->value);
  }
  // Perform a lookup based on the lower 4 bits of each lane. (Platform-dependent behavior for
  // non-ASCII values--may look up the lower 4 bits on some platforms, and return 0 on others.)
  simdjson_inline simd8<T> lookup_low_nibble_ascii(const simd8<T>& lookup_table) const {
    return lookup_16(lookup_table);
  }

  // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted
  // as a bitset). Passing a 0 value for mask would be equivalent to writing out
  // every byte to output. Only the first 16 - bitmask::count_ones(mask) bytes of the
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
    simd_t shufmask = (simd_t)(__vector unsigned long long){
        thintable_epi8[mask1], thintable_epi8[mask2]};
#else
    simd_t shufmask = (simd_t)(__vector unsigned long long){
        thintable_epi8[mask2], thintable_epi8[mask1]};
    shufmask = (simd_t)vec_reve((simd_t)shufmask);
#endif
    // we increment by 0x08 the second half of the mask
    shufmask = ((simd_t)shufmask) +
               ((simd_t)(__vector int){0, 0, 0x08080808, 0x08080808});

    // this is the version "nearly pruned"
    simd_t pruned = vec_perm(this->value, this->value, shufmask);
    // we still need to put the two halves together.
    // we compute the popcount of the first half:
    int pop1 = BitsSetTable256mul2[mask1];
    // then load the corresponding mask, what it does is to write
    // only the first pop1 bytes from the first 8 bytes, and then
    // it fills in with the bytes from the second 8 bytes + some filling
    // at the end.
    simd_t compactmask =
        vec_vsx_ld(0, reinterpret_cast<const uint8_t *>(pshufb_combine_table + pop1 * 8));
    simd_t answer = vec_perm(pruned, (simd_t)vec_splats(0), compactmask);
    vec_vsx_st(answer, 0, reinterpret_cast<simd_t *>(output));
  }
};

// Signed bytes
template <> struct simd8<int8_t> : base8_numeric<int8_t> {
  simdjson_inline simd8() : base8_numeric<int8_t>() {}
  simdjson_inline simd8(const simd_t _value)
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
      : simd8((simd_t)(__m128i){v0, v1, v2, v3, v4, v5, v6, v7,
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
    return (simd_t)vec_max((__m128i)this->value,
                            (__m128i)(simd_t)other);
  }
  simdjson_inline simd8<int8_t>
  min_val(const simd8<int8_t> other) const {
    return (simd_t)vec_min((__m128i)this->value,
                            (__m128i)(simd_t)other);
  }
  simdjson_inline simd8<bool>
  operator>(const simd8<int8_t> other) const {
    return (simd_t)vec_cmpgt((__m128i)this->value,
                              (__m128i)(simd_t)other);
  }
  simdjson_inline simd8<bool>
  operator<(const simd8<int8_t> other) const {
    return (simd_t)vec_cmplt((__m128i)this->value,
                              (__m128i)(simd_t)other);
  }
};

// Unsigned bytes
template <> struct simd8<uint8_t> : base8_numeric<uint8_t> {
  simdjson_inline simd8() : base8_numeric<uint8_t>() {}
  simdjson_inline simd8(const simd_t _value)
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
      : simd8((simd_t){v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12,
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
    return (simd_t)vec_adds(this->value, (simd_t)other);
  }
  simdjson_inline simd8<uint8_t>
  saturating_sub(const simd8<uint8_t> other) const {
    return (simd_t)vec_subs(this->value, (simd_t)other);
  }

  // Order-specific operations
  simdjson_inline simd8<uint8_t>
  max_val(const simd8<uint8_t> other) const {
    return (simd_t)vec_max(this->value, (simd_t)other);
  }
  simdjson_inline simd8<uint8_t>
  min_val(const simd8<uint8_t> other) const {
    return (simd_t)vec_min(this->value, (simd_t)other);
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
    return (simd_t)vec_cmpeq(this->value, (simd_t)vec_splats(uint8_t(0)));
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
    return vec_all_eq(this->value, (simd_t)vec_splats(0));
  }
  simdjson_inline bool any_bits_set_anywhere() const {
    return !bits_not_set_anywhere();
  }
  simdjson_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const {
    return vec_all_eq(vec_and(this->value, (simd_t)bits),
                      (simd_t)vec_splats(0));
  }
  simdjson_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const {
    return !bits_not_set_anywhere(bits);
  }
  template <int N> simdjson_inline simd8<uint8_t> shr() const {
    return simd8<uint8_t>(
        (simd_t)vec_sr(this->value, (simd_t)vec_splat_u8(N)));
  }
  template <int N> simdjson_inline simd8<uint8_t> shl() const {
    return simd8<uint8_t>(
        (simd_t)vec_sl(this->value, (simd_t)vec_splat_u8(N)));
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
  simdjson_inline simd8x64(simd8x64<T>&& o) noexcept = default;
  simdjson_inline simd8x64<T>& operator=(simd8x64<T>&& other) noexcept = default;

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
                             output + 16 - bitmask::count_ones(mask & 0xFFFF));
    this->chunks[2].compress(uint16_t(mask >> 32),
                             output + 32 - bitmask::count_ones(mask & 0xFFFFFFFF));
    this->chunks[3].compress(uint16_t(mask >> 48),
                             output + 48 - bitmask::count_ones(mask & 0xFFFFFFFFFFFF));
    return 64 - bitmask::count_ones(mask);
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

  simdjson_inline uint64_t eq(const simd8x64<T> &other) const {
    return simd8x64<bool>(this->chunks[0] == other.chunks[0],
                          this->chunks[1] == other.chunks[1],
                          this->chunks[2] == other.chunks[2],
                          this->chunks[3] == other.chunks[3])
        .to_bitmask();
  }

  simdjson_inline simd8x64<T> lookup_16(const simd8<T>& lookup_table) const {
    return {
      this->chunks[0].lookup_16(lookup_table),
      this->chunks[1].lookup_16(lookup_table),
      this->chunks[2].lookup_16(lookup_table),
      this->chunks[3].lookup_16(lookup_table),
    };
  }

  simdjson_inline simd8x64<T> lookup_low_nibble_ascii(const simd8<T>& lookup_table) const {
    return {
      this->chunks[0].lookup_low_nibble_ascii(lookup_table),
      this->chunks[1].lookup_low_nibble_ascii(lookup_table),
      this->chunks[2].lookup_low_nibble_ascii(lookup_table),
      this->chunks[3].lookup_low_nibble_ascii(lookup_table)
    };
  }

  simdjson_inline uint64_t lteq(const T m) const {
    const simd8<T> mask = simd8<T>::splat(m);
    return simd8x64<bool>(this->chunks[0] <= mask, this->chunks[1] <= mask,
                          this->chunks[2] <= mask, this->chunks[3] <= mask)
        .to_bitmask();
  }

  simdjson_inline simd8x64<T> operator&(const simd8x64<T>& other) const {
    return {
      this->chunks[0] & other.chunks[0],
      this->chunks[1] & other.chunks[1],
      this->chunks[2] & other.chunks[2],
      this->chunks[3] & other.chunks[3]
    };
  }

  simdjson_inline simd8x64<T> operator&(const simd8<T>& other) const {
    return {
      this->chunks[0] & other,
      this->chunks[1] & other,
      this->chunks[2] & other,
      this->chunks[3] & other
    };
  }

  simdjson_inline simd8x64<T> operator|(const simd8x64<T>& other) const {
    return  {
      this->chunks[0] | other.chunks[0],
      this->chunks[1] | other.chunks[1],
      this->chunks[2] | other.chunks[2],
      this->chunks[3] | other.chunks[3]
    };
  }

  simdjson_inline simd8x64<T> operator|(const simd8<T>& other) const {
    return  {
      this->chunks[0] | other,
      this->chunks[1] | other,
      this->chunks[2] | other,
      this->chunks[3] | other
    };
  }

  simdjson_inline simd8x64<T> operator^(const simd8x64<T>& other) const {
    return {
      this->chunks[0] ^ other.chunks[0],
      this->chunks[1] ^ other.chunks[1],
      this->chunks[2] ^ other.chunks[2],
      this->chunks[3] ^ other.chunks[3]
    };
  }

  simdjson_inline simd8x64<T> operator^(const simd8<T>& other) const {
    return {
      this->chunks[0] ^ other,
      this->chunks[1] ^ other,
      this->chunks[2] ^ other,
      this->chunks[3] ^ other
    };
  }

  simdjson_inline simd8x64<T> bit_andnot(const simd8x64<T>& other) const {
    return  {
      this->chunks[0].bit_andnot(other.chunks[0]),
      this->chunks[1].bit_andnot(other.chunks[1]),
      this->chunks[2].bit_andnot(other.chunks[2]),
      this->chunks[3].bit_andnot(other.chunks[3])
    };
  }

  simdjson_inline simd8x64<T> bit_andnot(const simd8<T>& other) const {
    return  {
      this->chunks[0].bit_andnot(other),
      this->chunks[1].bit_andnot(other),
      this->chunks[2].bit_andnot(other),
      this->chunks[3].bit_andnot(other)
    };
  }

  template <int N>
  simdjson_inline simd8x64<T> shr() const noexcept {
    return {
      this->chunks[0].template shr<N>(),
      this->chunks[1].template shr<N>(),
      this->chunks[2].template shr<N>(),
      this->chunks[3].template shr<N>()
    };
  }

  template <int N>
  simdjson_inline simd8x64<T> shl() const noexcept {
    return {
      this->chunks[0].template shl<N>(),
      this->chunks[1].template shl<N>(),
      this->chunks[2].template shl<N>(),
      this->chunks[3].template shl<N>()
    };
  }

  simdjson_inline simd8x64<bool> any_bits_set(const simd8<T>& bits) const {
    return {
      this->chunks[0].any_bits_set(bits),
      this->chunks[1].any_bits_set(bits),
      this->chunks[2].any_bits_set(bits),
      this->chunks[3].any_bits_set(bits)
    };
  }

  simdjson_inline simd8x64<bool> any_bits_set(const simd8x64<T>& bits) const {
    return {
      this->chunks[0].any_bits_set(bits.chunks[0]),
      this->chunks[1].any_bits_set(bits.chunks[1]),
      this->chunks[2].any_bits_set(bits.chunks[2]),
      this->chunks[3].any_bits_set(bits.chunks[3])
    };
  }

  simdjson_inline simd8x64<bool> no_bits_set(const simd8<T>& bits) const {
    return {
      this->chunks[0].no_bits_set(bits),
      this->chunks[1].no_bits_set(bits),
      this->chunks[2].no_bits_set(bits),
      this->chunks[3].no_bits_set(bits)
    };
  }

  simdjson_inline simd8x64<bool> no_bits_set(const simd8x64<T>& bits) const {
    return {
      this->chunks[0].no_bits_set(bits.chunks[0]),
      this->chunks[1].no_bits_set(bits.chunks[1]),
      this->chunks[2].no_bits_set(bits.chunks[2]),
      this->chunks[3].no_bits_set(bits.chunks[3])
    };
  }
}; // struct simd8x64<T>

} // namespace simd
} // namespace ppc64
} // namespace simdjson

#endif // SIMDJSON_PPC64_SIMD_INPUT_H
