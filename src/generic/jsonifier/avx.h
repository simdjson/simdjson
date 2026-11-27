// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#ifndef SIMDJSON_SRC_GENERIC_JSONIFIER_AVX_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSONIFIER_AVX_H
#include <generic/jsonifier/config.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_JSONIFIER_ANY_AVX

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsonifier {

simdjson_inline uint64_t op_cmp_eq(const __m128i value, const __m128i other) noexcept {
  return static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(value, other)));
}

simdjson_inline __m128i op_cmp_lt_raw(const __m128i value, const __m128i other) noexcept {
  const __m128i offset{_mm_set1_epi8(static_cast<char>(0x80))};
  return _mm_cmpgt_epi8(_mm_add_epi8(other, offset), _mm_add_epi8(value, offset));
}

simdjson_inline __m128i op_shuffle(const __m128i value, const __m128i other) noexcept {
  return _mm_shuffle_epi8(value, other);
}

simdjson_inline __m128i op_xor(const __m128i value, const __m128i other) noexcept {
  return _mm_xor_si128(value, other);
}

simdjson_inline __m128i op_and(const __m128i value, const __m128i other) noexcept {
  return _mm_and_si128(value, other);
}

simdjson_inline __m128i op_or(const __m128i value, const __m128i other) noexcept {
  return _mm_or_si128(value, other);
}

simdjson_inline void store_u(const __m128i value, void *__restrict storage_location) noexcept {
  _mm_storeu_si128(static_cast<__m128i *>(storage_location), value);
}

simdjson_inline uint64_t op_cmp_eq(const __m256i value, const __m256i other) noexcept {
  return static_cast<uint32_t>(_mm256_movemask_epi8(_mm256_cmpeq_epi8(value, other)));
}

simdjson_inline __m256i op_cmp_lt_raw(const __m256i value, const __m256i other) noexcept {
  const __m256i offset{_mm256_set1_epi8(static_cast<char>(0x80))};
  return _mm256_cmpgt_epi8(_mm256_add_epi8(other, offset), _mm256_add_epi8(value, offset));
}

simdjson_inline __m256i op_shuffle(const __m256i value, const __m256i other) noexcept {
  return _mm256_shuffle_epi8(value, other);
}

simdjson_inline __m256i op_xor(const __m256i value, const __m256i other) noexcept {
  return _mm256_xor_si256(value, other);
}

simdjson_inline __m256i op_and(const __m256i value, const __m256i other) noexcept {
  return _mm256_and_si256(value, other);
}

simdjson_inline __m256i op_or(const __m256i value, const __m256i other) noexcept {
  return _mm256_or_si256(value, other);
}

simdjson_inline void store_u(const __m256i value, void *__restrict storage_location) noexcept {
  _mm256_storeu_si256(static_cast<__m256i *>(storage_location), value);
}

#if SIMDJSON_JSONIFIER_AVX512

simdjson_inline uint64_t op_cmp_eq(const __m512i value, const __m512i other) noexcept {
  return static_cast<uint64_t>(_mm512_cmpeq_epi8_mask(value, other));
}

simdjson_inline __m512i op_cmp_lt_raw(const __m512i value, const __m512i other) noexcept {
  return _mm512_movm_epi8(_mm512_cmplt_epu8_mask(value, other));
}

simdjson_inline __m512i op_shuffle(const __m512i value, const __m512i other) noexcept {
  return _mm512_shuffle_epi8(value, other);
}

simdjson_inline __m512i op_xor(const __m512i value, const __m512i other) noexcept {
  return _mm512_xor_si512(value, other);
}

simdjson_inline __m512i op_and(const __m512i value, const __m512i other) noexcept {
  return _mm512_and_si512(value, other);
}

simdjson_inline __m512i op_or(const __m512i value, const __m512i other) noexcept {
  return _mm512_or_si512(value, other);
}

simdjson_inline void store_u(const __m512i value, void *__restrict storage_location) noexcept {
  _mm512_storeu_si512(storage_location, value);
}

#endif

template<uint64_t register_bytes> struct simd_loader;

template<> struct simd_loader<16> {
  simdjson_inline static __m128i load_u(const void *__restrict str) noexcept {
    return _mm_loadu_si128(static_cast<const __m128i *>(str));
  }

  simdjson_inline static __m128i load(const void *__restrict str) noexcept {
    return _mm_load_si128(static_cast<const __m128i *>(str));
  }

  simdjson_inline static __m128i splat(const uint8_t value) noexcept {
    return _mm_set1_epi8(static_cast<char>(value));
  }
};

template<> struct simd_loader<32> {
  simdjson_inline static __m256i load_u(const void *__restrict str) noexcept {
    return _mm256_loadu_si256(static_cast<const __m256i *>(str));
  }

  simdjson_inline static __m256i load(const void *__restrict str) noexcept {
    return _mm256_load_si256(static_cast<const __m256i *>(str));
  }

  simdjson_inline static __m256i splat(const uint8_t value) noexcept {
    return _mm256_set1_epi8(static_cast<char>(value));
  }
};

#if SIMDJSON_JSONIFIER_AVX512

template<> struct simd_loader<64> {
  simdjson_inline static __m512i load_u(const void *__restrict str) noexcept {
    return _mm512_loadu_si512(str);
  }

  simdjson_inline static __m512i load(const void *__restrict str) noexcept {
    return _mm512_load_si512(str);
  }

  simdjson_inline static __m512i splat(const uint8_t value) noexcept {
    return _mm512_set1_epi8(static_cast<char>(value));
  }
};

#endif

} // namespace jsonifier
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_JSONIFIER_ANY_AVX

#endif // SIMDJSON_SRC_GENERIC_JSONIFIER_AVX_H
