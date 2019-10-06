#ifndef SIMDJSON_WESTMERE_SIMD_INPUT_H
#define SIMDJSON_WESTMERE_SIMD_INPUT_H

#include "simdjson/common_defs.h"
#include "simdjson/portability.h"
#include "simdjson/simdjson.h"

#ifdef IS_X86_64

TARGET_WESTMERE
namespace simdjson::westmere {

struct simd_input {
  const __m128i chunks[4];

  really_inline simd_input()
      : chunks { __m128i(), __m128i(), __m128i(), __m128i() } {}

  really_inline simd_input(const __m128i chunk0, const __m128i chunk1, const __m128i chunk2, const __m128i chunk3)
      : chunks{chunk0, chunk1, chunk2, chunk3} {}

  really_inline simd_input(const uint8_t *ptr)
      : simd_input(
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 0)),
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 16)),
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 32)),
        _mm_loadu_si128(reinterpret_cast<const __m128i *>(ptr + 48))
      ) {}

  template <typename F>
  really_inline void each(F const& each_chunk) const {
    each_chunk(this->chunks[0]);
    each_chunk(this->chunks[1]);
    each_chunk(this->chunks[2]);
    each_chunk(this->chunks[3]);
  }

  template <typename F>
  really_inline simd_input map(F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0]),
      map_chunk(this->chunks[1]),
      map_chunk(this->chunks[2]),
      map_chunk(this->chunks[3])
    );
  }

  template <typename F>
  really_inline simd_input map(const simd_input b, F const& map_chunk) const {
    return simd_input(
      map_chunk(this->chunks[0], b.chunks[0]),
      map_chunk(this->chunks[1], b.chunks[1]),
      map_chunk(this->chunks[2], b.chunks[2]),
      map_chunk(this->chunks[3], b.chunks[3])
    );
  }

  template <typename F>
  really_inline __m128i reduce(F const& reduce_pair) const {
    __m128i r01 = reduce_pair(this->chunks[0], this->chunks[1]);
    __m128i r23 = reduce_pair(this->chunks[2], this->chunks[3]);
    return reduce_pair(r01, r23);
  }

  really_inline uint64_t to_bitmask() const {
    uint64_t r0 = static_cast<uint32_t>(_mm_movemask_epi8(this->chunks[0]));
    uint64_t r1 =                       _mm_movemask_epi8(this->chunks[1]);
    uint64_t r2 =                       _mm_movemask_epi8(this->chunks[2]);
    uint64_t r3 =                       _mm_movemask_epi8(this->chunks[3]);
    return r0 | (r1 << 16) | (r2 << 32) | (r3 << 48);
  }

  really_inline simd_input bit_or(const uint8_t m) const {
    const __m128i mask = _mm_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm_or_si128(a, mask);
    });
  }

  really_inline uint64_t eq(const uint8_t m) const {
    const __m128i mask = _mm_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm_cmpeq_epi8(a, mask);
    }).to_bitmask();
  }

  really_inline uint64_t lteq(const uint8_t m) const {
    const __m128i maxval = _mm_set1_epi8(m);
    return this->map( [&](auto a) {
      return _mm_cmpeq_epi8(_mm_max_epu8(maxval, a), maxval);
    }).to_bitmask();
  }

}; // struct simd_input

} // namespace simdjson::westmere
UNTARGET_REGION

#endif // IS_X86_64
#endif // SIMDJSON_WESTMERE_SIMD_INPUT_H
