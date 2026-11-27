// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#ifndef SIMDJSON_SRC_GENERIC_JSONIFIER_CONFIG_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSONIFIER_CONFIG_H
#include <generic/base.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <array>
#include <cstdint>
#include <cstring>
#include <utility>

#if SIMDJSON_JSONIFIER_ISA == 2
#define SIMDJSON_JSONIFIER_AVX512 1
#define SIMDJSON_JSONIFIER_ANY_AVX 1
#define SIMDJSON_JSONIFIER_NEON 0
#elif SIMDJSON_JSONIFIER_ISA == 1
#define SIMDJSON_JSONIFIER_AVX512 0
#define SIMDJSON_JSONIFIER_ANY_AVX 1
#define SIMDJSON_JSONIFIER_NEON 0
#elif SIMDJSON_JSONIFIER_ISA == 3
#define SIMDJSON_JSONIFIER_AVX512 0
#define SIMDJSON_JSONIFIER_ANY_AVX 0
#define SIMDJSON_JSONIFIER_NEON 1
#else
#error "Define SIMDJSON_JSONIFIER_ISA (1 AVX2, 2 AVX-512, 3 NEON) before including the Jsonifier stage 1."
#endif

#if defined(__clang__)
#define SIMDJSON_JSONIFIER_CLANG 1
#define SIMDJSON_JSONIFIER_GCC 0
#define SIMDJSON_JSONIFIER_MSVC 0
#elif defined(__GNUC__)
#define SIMDJSON_JSONIFIER_CLANG 0
#define SIMDJSON_JSONIFIER_GCC 1
#define SIMDJSON_JSONIFIER_MSVC 0
#else
#define SIMDJSON_JSONIFIER_CLANG 0
#define SIMDJSON_JSONIFIER_GCC 0
#define SIMDJSON_JSONIFIER_MSVC 1
#endif

#if defined(__SANITIZE_ADDRESS__)
#define SIMDJSON_JSONIFIER_ASAN 1
#elif defined(__has_feature)
#if __has_feature(address_sanitizer)
#define SIMDJSON_JSONIFIER_ASAN 1
#else
#define SIMDJSON_JSONIFIER_ASAN 0
#endif
#else
#define SIMDJSON_JSONIFIER_ASAN 0
#endif

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsonifier {

using read_buffer_ptr = const uint8_t *__restrict;
using write_buffer_ptr = uint8_t *__restrict;
using write_structural_index_ptr = uint32_t *__restrict;

#if SIMDJSON_JSONIFIER_AVX512 && SIMDJSON_JSONIFIER_GCC
inline constexpr uint64_t BLOCKS_PER_STEP{4};
#elif SIMDJSON_JSONIFIER_AVX512 && SIMDJSON_JSONIFIER_MSVC
inline constexpr uint64_t BLOCKS_PER_STEP{2};
#elif SIMDJSON_JSONIFIER_AVX512 && SIMDJSON_JSONIFIER_CLANG
inline constexpr uint64_t BLOCKS_PER_STEP{8};
#else
inline constexpr uint64_t BLOCKS_PER_STEP{4};
#endif

#if SIMDJSON_JSONIFIER_NEON && SIMDJSON_JSONIFIER_GCC
inline constexpr uint64_t TAPE_STEP{4};
#else
inline constexpr uint64_t TAPE_STEP{2};
#endif

#if SIMDJSON_JSONIFIER_ANY_AVX
using simd_int_128 = __m128i;
using simd_int_256 = __m256i;
using simd_int_512 = __m512i;
#if SIMDJSON_JSONIFIER_AVX512
using simd_int_t = simd_int_512;
#else
using simd_int_t = simd_int_256;
#endif
#else
using simd_int_128 = uint8x16_t;
using simd_int_256 = uint32_t;
using simd_int_512 = uint64_t;
using simd_int_t = simd_int_128;
#endif

inline constexpr uint64_t BYTES_PER_REGISTER{sizeof(simd_int_t)};
inline constexpr uint64_t REGISTERS_PER_BLOCK{64 / BYTES_PER_REGISTER};
inline constexpr uint64_t BYTES_PER_BLOCK{64};
inline constexpr uint64_t BYTES_PER_STEP{BLOCKS_PER_STEP * BYTES_PER_BLOCK};

static_assert(BYTES_PER_BLOCK % BYTES_PER_REGISTER == 0, "Register width must evenly divide the 64-byte block.");

template<uint64_t register_bytes> struct simd_register_selector {
  using type = void;
};

template<> struct simd_register_selector<16> {
  using type = simd_int_128;
};

template<> struct simd_register_selector<32> {
  using type = simd_int_256;
};

template<> struct simd_register_selector<64> {
  using type = simd_int_512;
};

template<uint64_t register_bytes> using simd_register_t = typename simd_register_selector<register_bytes>::type;

template<uint64_t register_count, uint64_t register_bytes = BYTES_PER_REGISTER> struct simd_register_array {
  using simd_type = simd_register_t<register_bytes>;

  template<uint64_t index> simdjson_inline void set(const simd_type value) noexcept {
    static_assert(index < register_count, "simd_register_array::set index out of range.");
    values[index] = value;
  }

  template<uint64_t index> simdjson_inline simd_type get() const noexcept {
    static_assert(index < register_count, "simd_register_array::get index out of range.");
    return values[index];
  }

  alignas(register_bytes) simd_type values[register_count];
};

using simd_array_t = simd_register_array<REGISTERS_PER_BLOCK>;

template<uint64_t register_count, uint64_t register_bytes> using pod_simd_array_t = simd_register_array<register_count, register_bytes>;

} // namespace jsonifier
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_JSONIFIER_CONFIG_H
