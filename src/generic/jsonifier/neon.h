// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#ifndef SIMDJSON_SRC_GENERIC_JSONIFIER_NEON_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSONIFIER_NEON_H
#include <generic/jsonifier/config.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_JSONIFIER_NEON

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsonifier {

simdjson_inline uint8x16_t op_cmp_lt_raw(const uint8x16_t value, const uint8x16_t other) noexcept {
  return vcgtq_u8(other, value);
}

simdjson_inline uint8x16_t op_xor(const uint8x16_t value, const uint8x16_t other) noexcept {
  return veorq_u8(value, other);
}

simdjson_inline uint8x16_t op_and(const uint8x16_t value, const uint8x16_t other) noexcept {
  return vandq_u8(value, other);
}

simdjson_inline uint8x16_t op_or(const uint8x16_t value, const uint8x16_t other) noexcept {
  return vorrq_u8(value, other);
}

simdjson_inline void store_u(const uint8x16_t value, void *__restrict storage_location) noexcept {
  vst1q_u8(static_cast<uint8_t *>(storage_location), value);
}

template<uint64_t register_bytes> struct simd_loader;

template<> struct simd_loader<16> {
  simdjson_inline static uint8x16_t load_u(const void *__restrict str) noexcept {
    return vld1q_u8(static_cast<const uint8_t *>(str));
  }

  simdjson_inline static uint8x16_t load(const void *__restrict str) noexcept {
    return vld1q_u8(static_cast<const uint8_t *>(str));
  }

  simdjson_inline static uint8x16_t splat(const uint8_t value) noexcept {
    return vdupq_n_u8(value);
  }
};

alignas(16) inline constexpr uint8_t NEON_BIT_MASK_BYTES[16]{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};

simdjson_inline uint8x16_t neon_bit_mask() noexcept {
  return vld1q_u8(NEON_BIT_MASK_BYTES);
}

} // namespace jsonifier
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_JSONIFIER_NEON

#endif // SIMDJSON_SRC_GENERIC_JSONIFIER_NEON_H
