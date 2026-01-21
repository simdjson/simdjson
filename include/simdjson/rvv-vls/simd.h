#ifndef SIMDJSON_RVV_VLS_SIMD_H
#define SIMDJSON_RVV_VLS_SIMD_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv-vls/base.h"
#include "simdjson/rvv-vls/bitmanipulation.h"
#include "simdjson/internal/simdprune_tables.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace rvv_vls {
namespace {
namespace simd {

#if __riscv_v_fixed_vlen >= 512
  static constexpr size_t VL8 = 512/8;
  using vint8_t   = vint8m1_t   __attribute__((riscv_rvv_vector_bits(512)));
  using vuint8_t  = vuint8m1_t  __attribute__((riscv_rvv_vector_bits(512)));
  using vbool_t    = vbool8_t  __attribute__((riscv_rvv_vector_bits(512/8)));
  using vbitmask_t = uint64_t;
#else
  static constexpr size_t VL8 = __riscv_v_fixed_vlen/8;
  using vint8_t   = vint8m1_t   __attribute__((riscv_rvv_vector_bits(__riscv_v_fixed_vlen)));
  using vuint8_t  = vuint8m1_t  __attribute__((riscv_rvv_vector_bits(__riscv_v_fixed_vlen)));
  using vbool_t    = vbool8_t  __attribute__((riscv_rvv_vector_bits(__riscv_v_fixed_vlen/8)));
  #if __riscv_v_fixed_vlen == 128
    using vbitmask_t = uint16_t;
  #elif __riscv_v_fixed_vlen == 256
    using vbitmask_t = uint32_t;
  #endif
#endif

#if __riscv_v_fixed_vlen == 128
  using vuint8x64_t = vuint8m4_t __attribute__((riscv_rvv_vector_bits(512)));
  using vboolx64_t    = vbool2_t  __attribute__((riscv_rvv_vector_bits(512/8)));
#elif __riscv_v_fixed_vlen == 256
  using vuint8x64_t = vuint8m2_t __attribute__((riscv_rvv_vector_bits(512)));
  using vboolx64_t    = vbool4_t  __attribute__((riscv_rvv_vector_bits(512/8)));
#else
  using vuint8x64_t = vuint8m1_t __attribute__((riscv_rvv_vector_bits(512)));
  using vboolx64_t    = vbool8_t  __attribute__((riscv_rvv_vector_bits(512/8)));
#endif

  template<typename T>
  struct simd8;

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool> {
    vbool_t value;
    using bitmask_t = vbitmask_t;
    static constexpr int SIZE = sizeof(value);

    simdjson_inline simd8(const vbool_t _value) : value(_value) {}
    simdjson_inline simd8() : simd8(__riscv_vmclr_m_b8(VL8)) {}
    simdjson_inline simd8(bool _value) : simd8(splat(_value)) {}

    simdjson_inline operator const vbool_t&() const { return value; }
    simdjson_inline operator vbool_t&() { return value; }

    static simdjson_inline simd8<bool> splat(bool _value) {
      return __riscv_vreinterpret_b8(__riscv_vmv_v_x_u64m1(((uint64_t)!_value)-1, 1));
    }

    simdjson_inline vbitmask_t to_bitmask() const {
#if __riscv_v_fixed_vlen == 128
      return __riscv_vmv_x(__riscv_vreinterpret_u16m1(value));
#elif __riscv_v_fixed_vlen == 256
      return __riscv_vmv_x(__riscv_vreinterpret_u32m1(value));
#else
      return __riscv_vmv_x(__riscv_vreinterpret_u64m1(value));
#endif
    }

    // Bit operations
    simdjson_inline simd8<bool> operator|(const simd8<bool> other) const {  return __riscv_vmor(*this, other, VL8); }
    simdjson_inline simd8<bool> operator&(const simd8<bool> other) const {  return __riscv_vmand(*this, other, VL8); }
    simdjson_inline simd8<bool> operator^(const simd8<bool> other) const {  return __riscv_vmxor(*this, other, VL8); }
    simdjson_inline simd8<bool> bit_andnot(const simd8<bool> other) const { return __riscv_vmandn(other, *this, VL8); }
    simdjson_inline simd8<bool> operator~() const { return __riscv_vmnot(*this, VL8); }
    simdjson_inline simd8<bool>& operator|=(const simd8<bool> other) { auto this_cast = static_cast<simd8<bool>*>(this); *this_cast = *this_cast | other; return *this_cast; }
    simdjson_inline simd8<bool>& operator&=(const simd8<bool> other) { auto this_cast = static_cast<simd8<bool>*>(this); *this_cast = *this_cast & other; return *this_cast; }
    simdjson_inline simd8<bool>& operator^=(const simd8<bool> other) { auto this_cast = static_cast<simd8<bool>*>(this); *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t> {

    vuint8_t value;
    static constexpr int SIZE = sizeof(value);

    simdjson_inline simd8(const vuint8_t _value) : value(_value) {}
    simdjson_inline simd8() : simd8(zero()) {}
    simdjson_inline simd8(const uint8_t values[VL8]) : simd8(load(values)) {}
    simdjson_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    simdjson_inline simd8(simd8<bool> mask) : value(__riscv_vmerge_vxm_u8m1(zero(), -1, (vbool_t)mask, VL8)) {}

    simdjson_inline operator const vuint8_t&() const { return this->value; }
    simdjson_inline operator vuint8_t&() { return this->value; }

    simdjson_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(vuint8_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}

    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_inline static simd8<uint8_t> repeat_16(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) {
      return simd8<uint8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    static simdjson_inline vuint8_t splat(uint8_t _value) { return __riscv_vmv_v_x_u8m1(_value, VL8); }
    static simdjson_inline vuint8_t zero() { return splat(0); }
    static simdjson_inline vuint8_t load(const uint8_t values[VL8]) { return __riscv_vle8_v_u8m1(values, VL8); }

    // Bit operations
    simdjson_inline simd8<uint8_t> operator|(const simd8<uint8_t> other) const { return  __riscv_vor_vv_u8m1(  value, other, VL8); }
    simdjson_inline simd8<uint8_t> operator&(const simd8<uint8_t> other) const { return  __riscv_vand_vv_u8m1( value, other, VL8); }
    simdjson_inline simd8<uint8_t> operator^(const simd8<uint8_t> other) const { return  __riscv_vxor_vv_u8m1( value, other, VL8); }
    simdjson_inline simd8<uint8_t> operator~() const { return __riscv_vnot_v_u8m1(value, VL8); }
#if __riscv_zvbb
    simdjson_inline simd8<uint8_t> bit_andnot(const simd8<uint8_t> other) const { return __riscv_vandn_vv_u8m1(other, value, VL8); }
#else
    simdjson_inline simd8<uint8_t> bit_andnot(const simd8<uint8_t> other) const { return other & ~*this; }
#endif
    simdjson_inline simd8<uint8_t>& operator|=(const simd8<uint8_t> other) { value = *this | other; return *this; }
    simdjson_inline simd8<uint8_t>& operator&=(const simd8<uint8_t> other) { value = *this & other; return *this; }
    simdjson_inline simd8<uint8_t>& operator^=(const simd8<uint8_t> other) { value = *this ^ other; return *this; }

    simdjson_inline simd8<bool> operator==(const simd8<uint8_t> other) const { return __riscv_vmseq(value, other, VL8); }
    simdjson_inline simd8<bool> operator==(uint8_t other) const { return __riscv_vmseq(value, other, VL8); }

    template<int N=1>
    simdjson_inline simd8<uint8_t> prev(const simd8<uint8_t> prev_chunk) const {
      return __riscv_vslideup(__riscv_vslidedown(prev_chunk, VL8-N, VL8), value, N, VL8);
    }

    // Store to array
    simdjson_inline void store(uint8_t dst[VL8]) const { return __riscv_vse8(dst, value, VL8); }

    // Saturated math
    simdjson_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return __riscv_vsaddu(value, other, VL8); }
    simdjson_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return __riscv_vssubu(value, other, VL8); }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_inline simd8<uint8_t> operator+(const simd8<uint8_t> other) const { return __riscv_vadd(value, other, VL8); }
    simdjson_inline simd8<uint8_t> operator-(const simd8<uint8_t> other) const { return __riscv_vsub(value, other, VL8); }
    simdjson_inline simd8<uint8_t>& operator+=(const simd8<uint8_t> other) { value = *this + other; return *this; }
    simdjson_inline simd8<uint8_t>& operator-=(const simd8<uint8_t> other) { value = *this - other; return *this; }

    // Order-specific operations
    simdjson_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return __riscv_vmsleu(value, other, VL8); }
    simdjson_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return __riscv_vmsgeu(value, other, VL8); }
    simdjson_inline simd8<bool> operator<(const simd8<uint8_t> other) const  { return __riscv_vmsltu(value, other, VL8); }
    simdjson_inline simd8<bool> operator>(const simd8<uint8_t> other) const  { return __riscv_vmsgtu(value, other, VL8); }

    // Same as >, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero.
    simdjson_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this > other); }
    // Same as <, but instead of guaranteeing all 1's == true, false = 0 and true = nonzero.
    simdjson_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return simd8<uint8_t>(*this < other); }

    // Bit-specific operations
    simdjson_inline bool any_bits_set_anywhere() const {
      return __riscv_vfirst(__riscv_vmsne(value, 0, VL8), VL8) >= 0;
    }
    simdjson_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return (*this & bits).any_bits_set_anywhere(); }
    template<int N>
    simdjson_inline simd8<uint8_t> shr() const { return __riscv_vsrl(value, N, VL8); }
    template<int N>
    simdjson_inline simd8<uint8_t> shl() const { return __riscv_vsll(value, N, VL8); }


    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return __riscv_vrgather(lookup_table, value, VL8);
    }

    // compress inactive elements, to match AVX-512 behavior
    template<typename L>
    simdjson_inline void compress(vbitmask_t mask, L * output) const {
      mask = (vbitmask_t)~mask;
#if __riscv_v_fixed_vlen == 128
      vbool8_t m = __riscv_vreinterpret_b8(__riscv_vmv_s_x_u16m1(mask, 1));
#elif __riscv_v_fixed_vlen == 256
      vbool8_t m = __riscv_vreinterpret_b8(__riscv_vmv_s_x_u32m1(mask, 1));
#else
      vbool8_t m = __riscv_vreinterpret_b8(__riscv_vmv_s_x_u64m1(mask, 1));
#endif
      __riscv_vse8_v_u8m1(output, __riscv_vcompress(value, m, VL8), count_ones(mask));
    }

    template<typename L>
    simdjson_inline simd8<L> lookup_16(
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
  struct simd8<int8_t> {
    vint8_t value;
    static constexpr int SIZE = sizeof(value);

    simdjson_inline simd8(const vint8_t _value) : value(_value) {}
    simdjson_inline simd8() : simd8(zero()) {}
    simdjson_inline simd8(const int8_t values[VL8]) : simd8(load(values)) {}
    simdjson_inline simd8(int8_t _value) : simd8(splat(_value)) {}

    simdjson_inline operator const vint8_t&() const { return this->value; }
    simdjson_inline operator vint8_t&() { return this->value; }

    simdjson_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8(vint8_t{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    }) {}

    // Repeat 16 values as many times as necessary (usually for lookup tables)
    simdjson_inline static simd8<int8_t> repeat_16(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) {
      return simd8<int8_t>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    static simdjson_inline vint8_t splat(int8_t _value) { return __riscv_vmv_v_x_i8m1(_value, VL8); }
    static simdjson_inline vint8_t zero() { return splat(0); }
    static simdjson_inline vint8_t load(const int8_t values[VL8]) { return __riscv_vle8_v_i8m1(values, VL8); }


    simdjson_inline void store(int8_t dst[VL8]) const { return __riscv_vse8(dst, value, VL8); }

    // Explicit conversion to/from unsigned
    simdjson_inline explicit simd8(const vuint8_t other): simd8(__riscv_vreinterpret_i8m1(other)) {}
    simdjson_inline explicit operator simd8<uint8_t>() const { return __riscv_vreinterpret_u8m1(value); }

    // Math
    simdjson_inline simd8<int8_t> operator+(const simd8<int8_t> other) const { return __riscv_vadd(value, other, VL8); }
    simdjson_inline simd8<int8_t> operator-(const simd8<int8_t> other) const { return __riscv_vsub(value, other, VL8); }
    simdjson_inline simd8<int8_t>& operator+=(const simd8<int8_t> other) { value = *this + other; return *this; }
    simdjson_inline simd8<int8_t>& operator-=(const simd8<int8_t> other) { value = *this - other; return *this; }

    // Order-sensitive comparisons
    simdjson_inline simd8<int8_t> max_val( const simd8<int8_t> other) const { return __riscv_vmax( value, other, VL8); }
    simdjson_inline simd8<int8_t> min_val( const simd8<int8_t> other) const { return __riscv_vmin( value, other, VL8); }
    simdjson_inline simd8<bool> operator>( const simd8<int8_t> other) const { return __riscv_vmsgt(value, other, VL8); }
    simdjson_inline simd8<bool> operator<( const simd8<int8_t> other) const { return __riscv_vmslt(value, other, VL8); }
    simdjson_inline simd8<bool> operator==(const simd8<int8_t> other) const { return __riscv_vmseq(value, other, VL8); }

    template<int N=1>
    simdjson_inline simd8<int8_t> prev(const simd8<int8_t> prev_chunk) const {
      return __riscv_vslideup(__riscv_vslidedown(prev_chunk, VL8-N, VL8), value, N, VL8);
    }

    // Perform a lookup assuming no value is larger than 16
    template<typename L>
    simdjson_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return __riscv_vrgather(lookup_table, value, VL8);
    }
    template<typename L>
    simdjson_inline simd8<L> lookup_16(
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

  template<typename T>
  struct simd8x64;
  template<>
  struct simd8x64<uint8_t> {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<uint8_t>);
    vuint8x64_t value;

#if __riscv_v_fixed_vlen >= 512
    template<int idx> simd8<uint8_t> get() const { return value; }
#else
    template<int idx> simd8<uint8_t> get() const { return __riscv_vget_u8m1(value, idx); }
#endif

    simdjson_inline operator const vuint8x64_t&() const { return this->value; }
    simdjson_inline operator vuint8x64_t&() { return this->value; }

    simd8x64(const simd8x64<uint8_t>& o) = delete; // no copy allowed
    simd8x64<uint8_t>& operator=(const simd8<uint8_t>& other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

#if __riscv_v_fixed_vlen == 128
    simdjson_inline simd8x64(const uint8_t *ptr, size_t n = 64) : value(__riscv_vle8_v_u8m4(ptr, n)) {}
#elif __riscv_v_fixed_vlen == 256
    simdjson_inline simd8x64(const uint8_t *ptr, size_t n = 64) : value(__riscv_vle8_v_u8m2(ptr, n)) {}
#else
    simdjson_inline simd8x64(const uint8_t *ptr, size_t n = 64) : value(__riscv_vle8_v_u8m1(ptr, n)) {}
#endif

    simdjson_inline void store(uint8_t ptr[64]) const {
      __riscv_vse8(ptr, value, 64);
    }

    simdjson_inline bool is_ascii() const {
#if __riscv_v_fixed_vlen == 128
      return __riscv_vfirst(__riscv_vmslt(__riscv_vreinterpret_i8m4(value), 0, 64), 64) < 0;
#elif __riscv_v_fixed_vlen == 256
      return __riscv_vfirst(__riscv_vmslt(__riscv_vreinterpret_i8m2(value), 0, 64), 64) < 0;
#else
      return __riscv_vfirst(__riscv_vmslt(__riscv_vreinterpret_i8m1(value), 0, 64), 64) < 0;
#endif
    }

    // compress inactive elements, to match AVX-512 behavior
    simdjson_inline uint64_t compress(uint64_t mask, uint8_t * output) const {
      mask = ~mask;
#if __riscv_v_fixed_vlen == 128
      vboolx64_t m = __riscv_vreinterpret_b2(__riscv_vmv_s_x_u64m1(mask, 1));
#elif __riscv_v_fixed_vlen == 256
      vboolx64_t m = __riscv_vreinterpret_b4(__riscv_vmv_s_x_u64m1(mask, 1));
#else
      vboolx64_t m = __riscv_vreinterpret_b8(__riscv_vmv_s_x_u64m1(mask, 1));
#endif
      size_t cnt = count_ones(mask);
      __riscv_vse8(output, __riscv_vcompress(value, m, 64), cnt);
      return cnt;
    }

    simdjson_inline uint64_t eq(const uint8_t m) const {
      return __riscv_vmv_x(__riscv_vreinterpret_u64m1(__riscv_vmseq(value, m, 64)));
    }

    simdjson_inline uint64_t lteq(const uint8_t m) const {
      return __riscv_vmv_x(__riscv_vreinterpret_u64m1(__riscv_vmsleu(value, m, 64)));
    }
  }; // struct simd8x64<uint8_t>

} // namespace simd
} // unnamed namespace
} // namespace rvv_vls
} // namespace simdjson

#endif // SIMDJSON_RVV_VLS_SIMD_H
