#ifndef SIMDJSON_LSX_SIMD_H
#define SIMDJSON_LSX_SIMD_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/lsx/base.h"
#include "simdjson/lsx/bitmanipulation.h"
#include "simdjson/internal/simdprune_tables.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace lsx {
namespace {
namespace simd {

  // Forward-declared so they can be used by splat and friends.
  template<typename Child>
  struct base {
    __m128i value;

    // Zero constructor
    simdjson_inline base() : value{__m128i()} {}

    // Conversion from SIMD register
    simdjson_inline base(const __m128i _value) : value(_value) {}

    // Conversion to SIMD register
    simdjson_inline operator const __m128i&() const { return this->value; }
    simdjson_inline operator __m128i&() { return this->value; }
    simdjson_inline operator const v16i8&() const { return (v16i8&)this->value; }
    simdjson_inline operator v16i8&() { return (v16i8&)this->value; }

    // Bit operations
    simdjson_inline Child operator|(const Child other) const { return __lsx_vor_v(*this, other); }
    simdjson_inline Child operator&(const Child other) const { return __lsx_vand_v(*this, other); }
    simdjson_inline Child operator^(const Child other) const { return __lsx_vxor_v(*this, other); }
    simdjson_inline Child bit_andnot(const Child other) const { return __lsx_vandn_v(other, *this); }
    simdjson_inline Child& operator|=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast | other; return *this_cast; }
    simdjson_inline Child& operator&=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast & other; return *this_cast; }
    simdjson_inline Child& operator^=(const Child other) { auto this_cast = static_cast<Child*>(this); *this_cast = *this_cast ^ other; return *this_cast; }
  };

  // Forward-declared so they can be used by splat and friends.
  template<typename T>
  struct simd8;

  template<typename T, typename Mask=simd8<bool>>
  struct base8: base<simd8<T>> {
    simdjson_inline base8() : base<simd8<T>>() {}
    simdjson_inline base8(const __m128i _value) : base<simd8<T>>(_value) {}

    friend simdjson_really_inline Mask operator==(const simd8<T> lhs, const simd8<T> rhs) { return __lsx_vseq_b(lhs, rhs); }

    static const int SIZE = sizeof(base<simd8<T>>::value);

    template<int N=1>
    simdjson_inline simd8<T> prev(const simd8<T> prev_chunk) const {
      return __lsx_vor_v(__lsx_vbsll_v(*this, N), __lsx_vbsrl_v(prev_chunk, 16 - N));
    }
  };

  // SIMD byte mask type (returned by things like eq and gt)
  template<>
  struct simd8<bool>: base8<bool> {
    static simdjson_inline simd8<bool> splat(bool _value) {
      return __lsx_vreplgr2vr_b(uint8_t(-(!!_value)));
    }

    simdjson_inline simd8() : base8() {}
    simdjson_inline simd8(const __m128i _value) : base8<bool>(_value) {}
    // Splat constructor
    simdjson_inline simd8(bool _value) : base8<bool>(splat(_value)) {}

    simdjson_inline int to_bitmask() const { return __lsx_vpickve2gr_w(__lsx_vmskltz_b(*this), 0); }
    simdjson_inline bool any() const { return 0 == __lsx_vpickve2gr_hu(__lsx_vmsknz_b(*this), 0); }
    simdjson_inline simd8<bool> operator~() const { return *this ^ true; }
  };

  template<typename T>
  struct base8_numeric: base8<T> {
    static simdjson_inline simd8<T> splat(T _value) { return __lsx_vreplgr2vr_b(_value); }
    static simdjson_inline simd8<T> zero() { return __lsx_vldi(0); }
    static simdjson_inline simd8<T> load(const T values[16]) {
      return __lsx_vld(reinterpret_cast<const __m128i *>(values), 0);
    }
    // Repeat 16 values as many times as necessary (usually for lookup tables)
    static simdjson_inline simd8<T> repeat_16(
      T v0,  T v1,  T v2,  T v3,  T v4,  T v5,  T v6,  T v7,
      T v8,  T v9,  T v10, T v11, T v12, T v13, T v14, T v15
    ) {
      return simd8<T>(
        v0, v1, v2, v3, v4, v5, v6, v7,
        v8, v9, v10,v11,v12,v13,v14,v15
      );
    }

    simdjson_inline base8_numeric() : base8<T>() {}
    simdjson_inline base8_numeric(const __m128i _value) : base8<T>(_value) {}

    // Store to array
    simdjson_inline void store(T dst[16]) const {
      return __lsx_vst(*this, reinterpret_cast<__m128i *>(dst), 0);
    }

    // Addition/subtraction are the same for signed and unsigned
    simdjson_inline simd8<T> operator+(const simd8<T> other) const { return __lsx_vadd_b(*this, other); }
    simdjson_inline simd8<T> operator-(const simd8<T> other) const { return __lsx_vsub_b(*this, other); }
    simdjson_inline simd8<T>& operator+=(const simd8<T> other) { *this = *this + other; return *static_cast<simd8<T>*>(this); }
    simdjson_inline simd8<T>& operator-=(const simd8<T> other) { *this = *this - other; return *static_cast<simd8<T>*>(this); }

    // Override to distinguish from bool version
    simdjson_inline simd8<T> operator~() const { return *this ^ 0xFFu; }

    // Perform a lookup assuming the value is between 0 and 16 (undefined behavior for out of range values)
    template<typename L>
    simdjson_inline simd8<L> lookup_16(simd8<L> lookup_table) const {
      return __lsx_vshuf_b(lookup_table, lookup_table, *this);
    }

    // Copies to 'output" all bytes corresponding to a 0 in the mask (interpreted as a bitset).
    // Passing a 0 value for mask would be equivalent to writing out every byte to output.
    // Only the first 16 - count_ones(mask) bytes of the result are significant but 16 bytes
    // get written.
    template<typename L>
    simdjson_inline void compress(uint16_t mask, L * output) const {
      using internal::thintable_epi8;
      using internal::BitsSetTable256mul2;
      using internal::pshufb_combine_table;
      // this particular implementation was inspired by haswell
      // lsx do it in 2 steps, first 8 bytes and then second 8 bytes...
      uint8_t mask1 = uint8_t(mask); // least significant 8 bits
      uint8_t mask2 = uint8_t(mask >> 8); // second least significant 8 bits
      // next line just loads the 64-bit values thintable_epi8[mask1] and
      // thintable_epi8[mask2] into a 128-bit register.
      __m128i shufmask = {int64_t(thintable_epi8[mask1]), int64_t(thintable_epi8[mask2]) + 0x0808080808080808};
      // this is the version "nearly pruned"
      __m128i pruned = __lsx_vshuf_b(*this, *this, shufmask);
      // we still need to put the  pieces back together.
      // we compute the popcount of the first words:
      int pop1 = BitsSetTable256mul2[mask1];
      // then load the corresponding mask
      __m128i compactmask = __lsx_vldx(reinterpret_cast<void*>(reinterpret_cast<unsigned long>(pshufb_combine_table)), pop1 * 8);
      __m128i answer = __lsx_vshuf_b(pruned, pruned, compactmask);
      __lsx_vst(answer, reinterpret_cast<uint8_t*>(output), 0);
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
  struct simd8<int8_t> : base8_numeric<int8_t> {
    simdjson_inline simd8() : base8_numeric<int8_t>() {}
    simdjson_inline simd8(const __m128i _value) : base8_numeric<int8_t>(_value) {}
    // Splat constructor
    simdjson_inline simd8(int8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_inline simd8(const int8_t values[16]) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_inline simd8(
      int8_t v0,  int8_t v1,  int8_t v2,  int8_t v3,  int8_t v4,  int8_t v5,  int8_t v6,  int8_t v7,
      int8_t v8,  int8_t v9,  int8_t v10, int8_t v11, int8_t v12, int8_t v13, int8_t v14, int8_t v15
    ) : simd8({
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

    // Order-sensitive comparisons
    simdjson_inline simd8<int8_t> max_val(const simd8<int8_t> other) const { return __lsx_vmax_b(*this, other); }
    simdjson_inline simd8<int8_t> min_val(const simd8<int8_t> other) const { return __lsx_vmin_b(*this, other); }
    simdjson_inline simd8<bool> operator>(const simd8<int8_t> other) const { return __lsx_vslt_b(other, *this); }
    simdjson_inline simd8<bool> operator<(const simd8<int8_t> other) const { return __lsx_vslt_b(*this, other); }
  };

  // Unsigned bytes
  template<>
  struct simd8<uint8_t>: base8_numeric<uint8_t> {
    simdjson_inline simd8() : base8_numeric<uint8_t>() {}
    simdjson_inline simd8(const __m128i _value) : base8_numeric<uint8_t>(_value) {}
    // Splat constructor
    simdjson_inline simd8(uint8_t _value) : simd8(splat(_value)) {}
    // Array constructor
    simdjson_inline simd8(const uint8_t values[16]) : simd8(load(values)) {}
    // Member-by-member initialization
    simdjson_inline simd8(
      uint8_t v0,  uint8_t v1,  uint8_t v2,  uint8_t v3,  uint8_t v4,  uint8_t v5,  uint8_t v6,  uint8_t v7,
      uint8_t v8,  uint8_t v9,  uint8_t v10, uint8_t v11, uint8_t v12, uint8_t v13, uint8_t v14, uint8_t v15
    ) : simd8(__m128i(v16u8{
      v0, v1, v2, v3, v4, v5, v6, v7,
      v8, v9, v10,v11,v12,v13,v14,v15
    })) {}
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

    // Saturated math
    simdjson_inline simd8<uint8_t> saturating_add(const simd8<uint8_t> other) const { return __lsx_vsadd_bu(*this, other); }
    simdjson_inline simd8<uint8_t> saturating_sub(const simd8<uint8_t> other) const { return __lsx_vssub_bu(*this, other); }

    // Order-specific operations
    simdjson_inline simd8<uint8_t> max_val(const simd8<uint8_t> other) const { return __lsx_vmax_bu(*this, other); }
    simdjson_inline simd8<uint8_t> min_val(const simd8<uint8_t> other) const { return __lsx_vmin_bu(other, *this); }
    // Same as >, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_inline simd8<uint8_t> gt_bits(const simd8<uint8_t> other) const { return this->saturating_sub(other); }
    // Same as <, but only guarantees true is nonzero (< guarantees true = -1)
    simdjson_inline simd8<uint8_t> lt_bits(const simd8<uint8_t> other) const { return other.saturating_sub(*this); }
    simdjson_inline simd8<bool> operator<=(const simd8<uint8_t> other) const { return other.max_val(*this) == other; }
    simdjson_inline simd8<bool> operator>=(const simd8<uint8_t> other) const { return other.min_val(*this) == other; }
    simdjson_inline simd8<bool> operator>(const simd8<uint8_t> other) const { return this->gt_bits(other).any_bits_set(); }
    simdjson_inline simd8<bool> operator<(const simd8<uint8_t> other) const { return this->lt_bits(other).any_bits_set(); }

    // Bit-specific operations
    simdjson_inline simd8<bool> bits_not_set() const { return *this == uint8_t(0); }
    simdjson_inline simd8<bool> bits_not_set(simd8<uint8_t> bits) const { return (*this & bits).bits_not_set(); }
    simdjson_inline simd8<bool> any_bits_set() const { return ~this->bits_not_set(); }
    simdjson_inline simd8<bool> any_bits_set(simd8<uint8_t> bits) const { return ~this->bits_not_set(bits); }
    simdjson_inline bool is_ascii() const { return 0 == __lsx_vpickve2gr_w(__lsx_vmskltz_b(*this), 0); }
    simdjson_inline bool bits_not_set_anywhere() const { return 0 == __lsx_vpickve2gr_hu(__lsx_vmsknz_b(*this), 0); }
    simdjson_inline bool any_bits_set_anywhere() const { return !bits_not_set_anywhere(); }
    simdjson_inline bool bits_not_set_anywhere(simd8<uint8_t> bits) const {
      return 0 == __lsx_vpickve2gr_hu(__lsx_vmsknz_b(__lsx_vand_v(*this, bits)), 0);
    }
    simdjson_inline bool any_bits_set_anywhere(simd8<uint8_t> bits) const { return !bits_not_set_anywhere(bits); }
    template<int N>
    simdjson_inline simd8<uint8_t> shr() const { return simd8<uint8_t>(__lsx_vsrli_b(*this, N)); }
    template<int N>
    simdjson_inline simd8<uint8_t> shl() const { return simd8<uint8_t>(__lsx_vslli_b(*this, N)); }
  };

  template<typename T>
  struct simd8x64 {
    static constexpr int NUM_CHUNKS = 64 / sizeof(simd8<T>);
    static_assert(NUM_CHUNKS == 4, "LSX kernel should use four registers per 64-byte block.");
    const simd8<T> chunks[NUM_CHUNKS];

    simd8x64(const simd8x64<T>& o) = delete; // no copy allowed
    simd8x64<T>& operator=(const simd8<T>& other) = delete; // no assignment allowed
    simd8x64() = delete; // no default constructor allowed

    simdjson_inline simd8x64(const simd8<T> chunk0, const simd8<T> chunk1, const simd8<T> chunk2, const simd8<T> chunk3) : chunks{chunk0, chunk1, chunk2, chunk3} {}
    simdjson_inline simd8x64(const T ptr[64]) : chunks{simd8<T>::load(ptr), simd8<T>::load(ptr+16), simd8<T>::load(ptr+32), simd8<T>::load(ptr+48)} {}

    simdjson_inline uint64_t compress(uint64_t mask, T * output) const {
      uint16_t mask1 = uint16_t(mask);
      uint16_t mask2 = uint16_t(mask >> 16);
      uint16_t mask3 = uint16_t(mask >> 32);
      uint16_t mask4 = uint16_t(mask >> 48);
      __m128i zcnt = __lsx_vpcnt_h(__m128i(v2u64{~mask, 0}));
      uint64_t zcnt1 = __lsx_vpickve2gr_hu(zcnt, 0);
      uint64_t zcnt2 = __lsx_vpickve2gr_hu(zcnt, 1);
      uint64_t zcnt3 = __lsx_vpickve2gr_hu(zcnt, 2);
      uint64_t zcnt4 = __lsx_vpickve2gr_hu(zcnt, 3);
      uint8_t *voutput = reinterpret_cast<uint8_t*>(output);
      // There should be a critical value which processes in scaler is faster.
      if (zcnt1)
        this->chunks[0].compress(mask1, reinterpret_cast<T*>(voutput));
      voutput += zcnt1;
      if (zcnt2)
        this->chunks[1].compress(mask2, reinterpret_cast<T*>(voutput));
      voutput += zcnt2;
      if (zcnt3)
        this->chunks[2].compress(mask3, reinterpret_cast<T*>(voutput));
      voutput += zcnt3;
      if (zcnt4)
        this->chunks[3].compress(mask4, reinterpret_cast<T*>(voutput));
      voutput += zcnt4;
      return reinterpret_cast<uint64_t>(voutput) - reinterpret_cast<uint64_t>(output);
    }

    simdjson_inline void store(T ptr[64]) const {
      this->chunks[0].store(ptr+sizeof(simd8<T>)*0);
      this->chunks[1].store(ptr+sizeof(simd8<T>)*1);
      this->chunks[2].store(ptr+sizeof(simd8<T>)*2);
      this->chunks[3].store(ptr+sizeof(simd8<T>)*3);
    }

    simdjson_inline uint64_t to_bitmask() const {
      __m128i mask1 = __lsx_vmskltz_b(this->chunks[0]);
      __m128i mask2 = __lsx_vmskltz_b(this->chunks[1]);
      __m128i mask3 = __lsx_vmskltz_b(this->chunks[2]);
      __m128i mask4 = __lsx_vmskltz_b(this->chunks[3]);
      mask1 = __lsx_vilvl_h(mask2, mask1);
      mask2 = __lsx_vilvl_h(mask4, mask3);
      return __lsx_vpickve2gr_du(__lsx_vilvl_w(mask2, mask1), 0);
    }

    simdjson_inline simd8<T> reduce_or() const {
      return (this->chunks[0] | this->chunks[1]) | (this->chunks[2] | this->chunks[3]);
    }

    simdjson_inline uint64_t eq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] == mask,
        this->chunks[1] == mask,
        this->chunks[2] == mask,
        this->chunks[3] == mask
      ).to_bitmask();
    }

    simdjson_inline uint64_t eq(const simd8x64<uint8_t> &other) const {
      return  simd8x64<bool>(
        this->chunks[0] == other.chunks[0],
        this->chunks[1] == other.chunks[1],
        this->chunks[2] == other.chunks[2],
        this->chunks[3] == other.chunks[3]
      ).to_bitmask();
    }

    simdjson_inline uint64_t lteq(const T m) const {
      const simd8<T> mask = simd8<T>::splat(m);
      return  simd8x64<bool>(
        this->chunks[0] <= mask,
        this->chunks[1] <= mask,
        this->chunks[2] <= mask,
        this->chunks[3] <= mask
      ).to_bitmask();
    }
  }; // struct simd8x64<T>

} // namespace simd
} // unnamed namespace
} // namespace lsx
} // namespace simdjson

#endif // SIMDJSON_LSX_SIMD_H
