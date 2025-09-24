#ifndef SIMDJSON_RVV_SIMD_H
#define SIMDJSON_RVV_SIMD_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/rvv/base.h"
#include "simdjson/rvv/intrinsics.h"
#include "simdjson/internal/simdprune_tables.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace rvv {
namespace {
namespace simd {

// --------------------  forward --------------------
template<typename T> struct simd8;
template<typename T> struct simd8x64;

// --------------------  base<Child>  --------------------
template<typename Child>
struct base {
  vuint8m1_t value;          // 128-bit fixed view
  static constexpr size_t N = 16; // keep 128-bit throughout

  simdjson_inline base() : value{} {}
  simdjson_inline base(vuint8m1_t v) : value(v) {}
  simdjson_inline operator vuint8m1_t() const { return value; }
  simdjson_inline operator vuint8m1_t&()       { return value; }

  simdjson_inline Child operator|(const Child o) const { return __riscv_vor_vv_u8m1(value, o, N); }
  simdjson_inline Child operator&(const Child o) const { return __riscv_vand_vv_u8m1(value, o, N); }
  simdjson_inline Child operator^(const Child o) const { return __riscv_vxor_vv_u8m1(value, o, N); }
  simdjson_inline Child bit_andnot(const Child o) const { return __riscv_vandn_vv_u8m1(value, o, N); }

  simdjson_inline Child& operator|=(const Child o) { auto* c = static_cast<Child*>(this); *c = *c | o; return *c; }
  simdjson_inline Child& operator&=(const Child o) { auto* c = static_cast<Child*>(this); *c = *c & o; return *c; }
  simdjson_inline Child& operator^=(const Child o) { auto* c = static_cast<Child*>(this); *c = *c ^ o; return *c; }
};

// --------------------  base8<T,Mask>  --------------------
template<typename T, typename Mask = simd8<bool>>
struct base8 : base<simd8<T>> {
  using base<simd8<T>>::value;
  using bitmask_t = uint32_t;
  static constexpr int SIZE = 16;

  simdjson_inline base8() = default;
  simdjson_inline base8(vuint8m1_t v) : base<simd8<T>>(v) {}

  friend simdjson_inline Mask operator==(const simd8<T> a, const simd8<T> b) {
    return __riscv_vmseq_vv_u8m1_b8(a, b, SIZE);
  }

  template<int N=1>
  simdjson_inline simd8<T> prev(const simd8<T> prev) const {
    return __riscv_vslideup_vx_u8m1(prev, value, SIZE - N, 2 * SIZE);
  }
};

// --------------------  simd8<bool>  --------------------
template<>
struct simd8<bool> : base8<bool> {
  simdjson_inline simd8() : base8(__riscv_vmv_v_x_u8m1(0, N)) {}
  simdjson_inline simd8(vuint8m1_t v) : base8(v) {}
  static simdjson_inline simd8<bool> splat(bool b) {
    return __riscv_vmv_v_x_u8m1(uint8_t(-b), N);
  }
  simdjson_inline simd8(bool b) : simd8(splat(b)) {}

  simdjson_inline int to_bitmask() const {
    vuint8m1_t bits = __riscv_vand_vx_u8m1(value, 0xFFu, N);
    vbool8_t   mask = __riscv_vmseq_vx_u8m1_b8(bits, 0xFFu, N);
    return __riscv_vcpop_m_b8(mask, N);
  }
  simdjson_inline bool any() const { return to_bitmask() != 0; }
  simdjson_inline simd8<bool> operator~() const { return *this ^ true; }
};

// --------------------  simd8<uint8_t>  --------------------
template<>
struct simd8<uint8_t> : base8<uint8_t> {
  simdjson_inline simd8() = default;
  simdjson_inline simd8(vuint8m1_t v) : base8(v) {}
  static simdjson_inline simd8<uint8_t> splat(uint8_t v) { return __riscv_vmv_v_x_u8m1(v, N); }
  static simdjson_inline simd8<uint8_t> zero() { return splat(0); }
  static simdjson_inline simd8<uint8_t> load(const uint8_t* p) {
    return __riscv_vle8_v_u8m1(p, N);
  }
  simdjson_inline simd8(uint8_t v) : simd8(splat(v)) {}
  simdjson_inline simd8(const uint8_t* p) : simd8(load(p)) {}

  simdjson_inline void store(uint8_t* p) const { __riscv_vse8_v_u8m1(p, value, N); }

  simdjson_inline simd8<uint8_t> operator+(const simd8<uint8_t> o) const { return __riscv_vadd_vv_u8m1(value, o, N); }
  simdjson_inline simd8<uint8_t> operator-(const simd8<uint8_t> o) const { return __riscv_vsub_vv_u8m1(value, o, N); }
  simdjson_inline simd8<uint8_t>& operator+=(const simd8<uint8_t> o) { *this = *this + o; return *this; }
  simdjson_inline simd8<uint8_t>& operator-=(const simd8<uint8_t> o) { *this = *this - o; return *this; }

  simdjson_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> o) const { return __riscv_vsaddu_vv_u8m1(value, o, N); }
  simdjson_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> o) const { return __riscv_vssubu_vv_u8m1(value, o, N); }

  simdjson_inline simd8<uint8_t> max_val(const simd8<uint8_t> o) const { return __riscv_vmaxu_vv_u8m1(value, o, N); }
  simdjson_inline simd8<uint8_t> min_val(const simd8<uint8_t> o) const { return __riscv_vminu_vv_u8m1(value, o, N); }

  simdjson_inline simd8<bool> operator>(const simd8<uint8_t> o) const { return __riscv_vmsgtu_vv_u8m1_b8(value, o, N); }
  simdjson_inline simd8<bool> operator<(const simd8<uint8_t> o) const { return __riscv_vmsgtu_vv_u8m1_b8(o, value, N); }
  simdjson_inline simd8<bool> operator<=(const simd8<uint8_t> o) const { return __riscv_vmsleu_vv_u8m1_b8(value, o, N); }
  simdjson_inline simd8<bool> operator>=(const simd8<uint8_t> o) const { return __riscv_vmsleu_vv_u8m1_b8(o, value, N); }

  simdjson_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> o) const { return *this - max_val(o); }
  simdjson_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> o) const { return o - max_val(*this); }

  simdjson_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const {
    return (__riscv_vand_vv_u8m1(value, bits, N)).any_bits_set();
  }
  simdjson_inline bool any_bits_set_anywhere() const {
    vbool8_t m = __riscv_vmneq_vx_u8m1_b8(value, 0, N);
    return __riscv_vcpop_m_b8(m, N) != 0;
  }
  template<int NSHIFT>
  simdjson_inline simd8<uint8_t> shr() const { return __riscv_vsrl_vx_u8m1(value, NSHIFT, N); }
  template<int NSHIFT>
  simdjson_inline simd8<uint8_t> shl() const { return __riscv_vsll_vx_u8m1(value, NSHIFT, N); }

  template<typename L>
  simdjson_inline simd8<L> lookup_16(simd8<L> table) const {
    return table.apply_lookup_16_to(*this);
  }

  template<typename L>
  simdjson_inline void compress(uint32_t mask, L* out) const {
    using internal::thintable_epi8;
    using internal::BitsSetTable256mul2;
    using internal::pshufb_combine_table;
    uint8_t m0 = uint8_t(mask);
    uint8_t m1 = uint8_t(mask >> 8);
    vuint8m1_t shuf = __riscv_vle8_v_u8m1(&thintable_epi8[m0], 8);
    shuf = __riscv_vslideup_vx_u8m1(shuf,
                   __riscv_vle8_v_u8m1(&thintable_epi8[m1], 8), 8, 16);
    shuf = __riscv_vadd_vx_u8m1(shuf, 0x08, 16);
    vuint8m1_t pruned = __riscv_vrgather_vv_u8m1(value, shuf, 16);
    int pop = BitsSetTable256mul2[m0];
    vuint8m1_t compact = __riscv_vle8_v_u8m1(&pshufb_combine_table[pop * 8], 16);
    vuint8m1_t ans = __riscv_vrgather_vv_u8m1(pruned, compact, 16);
    __riscv_vse8_v_u8m1(reinterpret_cast<uint8_t*>(out), ans, 16);
  }
};

// --------------------  simd8<int8_t>  --------------------
template<>
struct simd8<int8_t> {
  vint8m1_t value;
  static constexpr size_t N = 16;

  static simdjson_inline simd8<int8_t> splat(int8_t v) { return __riscv_vmv_v_x_i8m1(v, N); }
  static simdjson_inline simd8<int8_t> zero() { return splat(0); }
  static simdjson_inline simd8<int8_t> load(const int8_t* p) { return __riscv_vle8_v_i8m1(p, N); }

  simdjson_inline simd8() : simd8(zero()) {}
  simdjson_inline simd8(int8_t v) : simd8(splat(v)) {}
  simdjson_inline simd8(const int8_t* p) : simd8(load(p)) {}
  simdjson_inline simd8(vint8m1_t v) : value(v) {}
  simdjson_inline operator vint8m1_t() const { return value; }

  simdjson_inline void store(int8_t* p) const { __riscv_vse8_v_i8m1(p, value, N); }

  simdjson_inline simd8<int8_t> operator+(const simd8<int8_t> o) const { return __riscv_vadd_vv_i8m1(value, o, N); }
  simdjson_inline simd8<int8_t> operator-(const simd8<int8_t> o) const { return __riscv_vsub_vv_i8m1(value, o, N); }
  simdjson_inline simd8<int8_t>& operator+=(const simd8<int8_t> o) { *this = *this + o; return *this; }
  simdjson_inline simd8<int8_t>& operator-=(const simd8<int8_t> o) { *this = *this - o; return *this; }

  simdjson_inline simd8<int8_t> max_val(const simd8<int8_t> o) const { return __riscv_vmax_vv_i8m1(value, o, N); }
  simdjson_inline simd8<int8_t> min_val(const simd8<int8_t> o) const { return __riscv_vmin_vv_i8m1(value, o, N); }
  simdjson_inline simd8<bool> operator>(const simd8<int8_t> o) const { return __riscv_vmsgt_vv_i8m1_b8(value, o, N); }
  simdjson_inline simd8<bool> operator<(const simd8<int8_t> o) const { return __riscv_vmsgt_vv_i8m1_b8(o, value, N); }
  simdjson_inline simd8<bool> operator==(const simd8<int8_t> o) const { return __riscv_vmseq_vv_i8m1_b8(value, o, N); }

  template<int NSHIFT>
  simdjson_inline simd8<int8_t> prev(const simd8<int8_t> prev_chunk) const {
    return __riscv_vslideup_vx_i8m1(prev_chunk, value, N - NSHIFT, 2 * N);
  }

  template<typename L>
  simdjson_inline simd8<L> lookup_16(simd8<L> table) const {
    return table.apply_lookup_16_to(*this);
  }
};

// --------------------  simd8x64<T>  --------------------
template<typename T>
struct simd8x64 {
  static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
  static_assert(NUM_CHUNKS == 4, "RVV kernel uses 4*128-bit chunks per 64-byte block.");
  const simd8<T> chunks[4];

  simd8x64(const simd8x64&) = delete;
  simd8x64& operator=(const simd8x64&) = delete;
  simd8x64() = delete;

  simdjson_inline simd8x64(const simd8<T> c0, const simd8<T> c1, const simd8<T> c2, const simd8<T> c3)
    : chunks{c0, c1, c2, c3} {}
  simdjson_inline simd8x64(const T* ptr)
    : chunks{ simd8<T>::load(ptr),
              simd8<T>::load(ptr + 16),
              simd8<T>::load(ptr + 32),
              simd8<T>::load(ptr + 48) } {}

  simdjson_inline void store(T* ptr) const {
    chunks[0].store(ptr);
    chunks[1].store(ptr + 16);
    chunks[2].store(ptr + 32);
    chunks[3].store(ptr + 48);
  }

  simdjson_inline simd8<T> reduce_or() const {
    return (chunks[0] | chunks[1]) | (chunks[2] | chunks[3]);
  }

  simdjson_inline uint64_t to_bitmask() const {
    uint16_t m0 = uint16_t(chunks[0].to_bitmask());
    uint16_t m1 = uint16_t(chunks[1].to_bitmask());
    uint16_t m2 = uint16_t(chunks[2].to_bitmask());
    uint16_t m3 = uint16_t(chunks[3].to_bitmask());
    return m0 | (uint64_t(m1) << 16) | (uint64_t(m2) << 32) | (uint64_t(m3) << 48);
  }

  simdjson_inline uint64_t eq(const T m) const {
    simd8<T> spl = simd8<T>::splat(m);
    return simd8x64<bool>(chunks[0] == spl,
                          chunks[1] == spl,
                          chunks[2] == spl,
                          chunks[3] == spl).to_bitmask();
  }

  simdjson_inline uint64_t lteq(const T m) const {
    simd8<T> spl = simd8<T>::splat(m);
    return simd8x64<bool>(chunks[0] <= spl,
                          chunks[1] <= spl,
                          chunks[2] <= spl,
                          chunks[3] <= spl).to_bitmask();
  }

  simdjson_inline uint64_t compress(uint64_t mask, T* out) const {
    using internal::BitsSetTable256mul2;
    uint32_t m0 = uint32_t(mask);
    uint32_t m1 = uint32_t(mask >> 16);
    uint32_t m2 = uint32_t(mask >> 32);
    uint32_t m3 = uint32_t(mask >> 48);

    int pop0 = BitsSetTable256mul2[uint8_t(m0)];
    int pop1 = BitsSetTable256mul2[uint8_t(m1)];
    int pop2 = BitsSetTable256mul2[uint8_t(m2)];

    chunks[0].compress(uint16_t(m0), out);
    chunks[1].compress(uint16_t(m1), out + 16 - pop0);
    chunks[2].compress(uint16_t(m2), out + 32 - pop0 - pop1);
    chunks[3].compress(uint16_t(m3), out + 48 - pop0 - pop1 - pop2);
    return 64 - __builtin_popcountll(mask);
  }
};

} // namespace simd
} // unnamed namespace
} // namespace rvv
} // namespace simdjson

#endif // SIMDJSON_RVV_SIMD_H