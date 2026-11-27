// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#ifndef SIMDJSON_SRC_GENERIC_JSONIFIER_NEON_STAGE1_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSONIFIER_NEON_STAGE1_H
#include <generic/jsonifier/neon.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_JSONIFIER_NEON

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsonifier {

simdjson_inline uint8x16_t to_partial_bitmask(const uint8x16_t match0, const uint8x16_t match1, const uint8x16_t match2, const uint8x16_t match3,
  const uint8x16_t bit_mask) noexcept {
  const uint8x16_t sum0{vpaddq_u8(vandq_u8(match0, bit_mask), vandq_u8(match1, bit_mask))};
  const uint8x16_t sum1{vpaddq_u8(vandq_u8(match2, bit_mask), vandq_u8(match3, bit_mask))};
  return vpaddq_u8(sum0, sum1);
}

simdjson_inline uint8x16_t ws_match(const uint8x16_t d, const uint8x16_t whitespace_table) noexcept {
  return vqtbx1q_u8(vceqq_u8(d, vdupq_n_u8(uint8_t(' '))), whitespace_table, d);
}

simdjson_inline uint8x16_t op_match(const uint8x16_t d, const uint8x16_t op_table) noexcept {
  return vceqq_u8(vqtbl1q_u8(op_table, vshrq_n_u8(vaddq_u8(d, vdupq_n_u8(3)), 4)), d);
}

struct ws_collector {
  simdjson_inline static uint64_t impl(const simd_array_t in01, const simd_int_t whitespace_table) noexcept {
    const uint8x16_t bit_mask{neon_bit_mask()};
    uint8x16_t ws_sum{to_partial_bitmask(ws_match(in01.get<0>(), whitespace_table), ws_match(in01.get<1>(), whitespace_table),
      ws_match(in01.get<2>(), whitespace_table), ws_match(in01.get<3>(), whitespace_table), bit_mask)};
    ws_sum = vpaddq_u8(ws_sum, ws_sum);
    return vgetq_lane_u64(vreinterpretq_u64_u8(ws_sum), 0);
  }
};

struct op_collector {
  simdjson_inline static uint64_t impl(const simd_array_t in01, const simd_int_t op_table, const simd_int_t) noexcept {
    const uint8x16_t bit_mask{neon_bit_mask()};
    uint8x16_t op_sum{to_partial_bitmask(op_match(in01.get<0>(), op_table), op_match(in01.get<1>(), op_table), op_match(in01.get<2>(), op_table),
      op_match(in01.get<3>(), op_table), bit_mask)};
    op_sum = vpaddq_u8(op_sum, op_sum);
    return vgetq_lane_u64(vreinterpretq_u64_u8(op_sum), 0);
  }
};

struct ws_op_collector {
  simdjson_inline static uint64x2_t impl(const simd_array_t in01, const simd_int_t op_table, const simd_int_t, const simd_int_t whitespace_table) noexcept {
    const uint8x16_t bit_mask{neon_bit_mask()};
    const uint8x16_t d00{in01.get<0>()};
    const uint8x16_t d01{in01.get<1>()};
    const uint8x16_t d02{in01.get<2>()};
    const uint8x16_t d03{in01.get<3>()};
    const uint8x16_t ws_sum{to_partial_bitmask(ws_match(d00, whitespace_table), ws_match(d01, whitespace_table), ws_match(d02, whitespace_table),
      ws_match(d03, whitespace_table), bit_mask)};
    const uint8x16_t op_sum{to_partial_bitmask(op_match(d00, op_table), op_match(d01, op_table), op_match(d02, op_table), op_match(d03, op_table), bit_mask)};
    return vreinterpretq_u64_u8(vpaddq_u8(ws_sum, op_sum));
  }
};

template<uint64_t register_count> struct scalar_bitmask_collector {
  using simd_array_type = pod_simd_array_t<register_count, 16>;

  simdjson_inline static uint64_t impl(const simd_array_type matches) noexcept {
    const uint8x16_t bit_mask{neon_bit_mask()};
    if constexpr (register_count == 1) {
      const uint8x16_t masked{vandq_u8(matches.template get<0>(), bit_mask)};
      uint8x16_t sum{vpaddq_u8(masked, masked)};
      sum = vpaddq_u8(sum, sum);
      sum = vpaddq_u8(sum, sum);
      return static_cast<uint64_t>(vgetq_lane_u16(vreinterpretq_u16_u8(sum), 0));
    } else {
      uint8x16_t sum{vpaddq_u8(vandq_u8(matches.template get<0>(), bit_mask), vandq_u8(matches.template get<1>(), bit_mask))};
      sum = vpaddq_u8(sum, sum);
      sum = vpaddq_u8(sum, sum);
      return static_cast<uint64_t>(vgetq_lane_u32(vreinterpretq_u32_u8(sum), 0));
    }
  }

  static_assert(register_count == 1 || register_count == 2, "NEON narrow blocks hold one or two registers.");
};

template<uint64_t register_bytes, uint64_t register_count> struct pod_cmp_eq_op {
  using simd_array_type = pod_simd_array_t<register_count, register_bytes>;

  simdjson_inline static uint64_t impl(const simd_array_type in01, const uint8x16_t rhs_broadcast) noexcept {
    simd_array_type matches;
    matches.template set<0>(vceqq_u8(in01.template get<0>(), rhs_broadcast));
    if constexpr (register_count > 1) {
      matches.template set<1>(vceqq_u8(in01.template get<1>(), rhs_broadcast));
    }
    return scalar_bitmask_collector<register_count>::impl(matches);
  }

  static_assert(register_bytes == 16 && (register_count == 1 || register_count == 2), "NEON narrow blocks hold one or two registers.");
};

template<uint64_t register_bytes, uint64_t register_count> struct pod_ws_collector {
  using simd_array_type = pod_simd_array_t<register_count, register_bytes>;

  simdjson_inline static uint64_t impl(const simd_array_type in01, const uint8x16_t whitespace_table) noexcept {
    if constexpr (register_count == REGISTERS_PER_BLOCK) {
      return ws_collector::impl(in01, whitespace_table);
    } else {
      simd_array_type matches;
      matches.template set<0>(ws_match(in01.template get<0>(), whitespace_table));
      if constexpr (register_count > 1) {
        matches.template set<1>(ws_match(in01.template get<1>(), whitespace_table));
      }
      return scalar_bitmask_collector<register_count>::impl(matches);
    }
  }
};

template<uint64_t register_bytes, uint64_t register_count> struct scalar_op_collector {
  using simd_array_type = pod_simd_array_t<register_count, register_bytes>;

  simdjson_inline static uint64_t impl(const simd_array_type in01, const uint8x16_t op_table, const uint8x16_t space_mask) noexcept {
    if constexpr (register_count == REGISTERS_PER_BLOCK) {
      return op_collector::impl(in01, op_table, space_mask);
    } else {
      simd_array_type matches;
      matches.template set<0>(op_match(in01.template get<0>(), op_table));
      if constexpr (register_count > 1) {
        matches.template set<1>(op_match(in01.template get<1>(), op_table));
      }
      return scalar_bitmask_collector<register_count>::impl(matches);
    }
  }
};

template<typename rope_block> struct rope_detector : rope_block {
  simdjson_inline void next(const simd_array_t in01, const simd_int_t bs_register, const simd_int_t quote_register) noexcept {
    const uint8x16_t bit_mask{neon_bit_mask()};
    const uint8x16_t d0{in01.get<0>()};
    const uint8x16_t d1{in01.get<1>()};
    const uint8x16_t d2{in01.get<2>()};
    const uint8x16_t d3{in01.get<3>()};
    const uint8x16_t backslash_partial{
      to_partial_bitmask(vceqq_u8(d0, bs_register), vceqq_u8(d1, bs_register), vceqq_u8(d2, bs_register), vceqq_u8(d3, bs_register), bit_mask)};
    const uint8x16_t quotes_partial{
      to_partial_bitmask(vceqq_u8(d0, quote_register), vceqq_u8(d1, quote_register), vceqq_u8(d2, quote_register), vceqq_u8(d3, quote_register), bit_mask)};
    const uint64x2_t paired_masks{vreinterpretq_u64_u8(vpaddq_u8(backslash_partial, quotes_partial))};
    const uint64_t escaped{next_escape_and_terminal_code(vgetq_lane_u64(paired_masks, 0))};
    const uint64_t quotes{vgetq_lane_u64(paired_masks, 1) & ~escaped};
    rope_block::escaped = escaped;
    rope_block::quotes = quotes;
    finish_next();
  }

  template<uint64_t register_bytes, uint64_t register_count> simdjson_inline void next_scalar(const pod_simd_array_t<register_count, register_bytes> in01,
    const simd_register_t<register_bytes> bs_register, const simd_register_t<register_bytes> quote_register) noexcept {
    if constexpr (register_count == REGISTERS_PER_BLOCK) {
      next(in01, bs_register, quote_register);
    } else {
      const uint64_t escaped{next_escape_and_terminal_code(pod_cmp_eq_op<register_bytes, register_count>::impl(in01, bs_register))};
      const uint64_t quotes{pod_cmp_eq_op<register_bytes, register_count>::impl(in01, quote_register) & ~escaped};
      rope_block::escaped = escaped;
      rope_block::quotes = quotes;
      finish_next();
    }
  }

  simdjson_inline uint64_t next_escape_and_terminal_code(const uint64_t backslash) noexcept {
    if (!backslash) {
      const uint64_t escaped{next_is_escaped};
      next_is_escaped = 0;
      return escaped;
    }
    const uint64_t escape_and_terminal_code{next_escape_and_terminal_code_impl(backslash & ~next_is_escaped)};
    const uint64_t escaped{escape_and_terminal_code ^ (backslash | next_is_escaped)};
    next_is_escaped = (escape_and_terminal_code & backslash) >> 63;
    return escaped;
  }

  simdjson_inline static uint64_t next_escape_and_terminal_code_impl(const uint64_t potential_escape) noexcept {
    constexpr uint64_t ODD_BITS{0xAAAAAAAAAAAAAAAAULL};
    const uint64_t maybe_escaped_and_odd_bits{(potential_escape << 1) | ODD_BITS};
    const uint64_t even_series_codes_and_odd_bits{maybe_escaped_and_odd_bits - potential_escape};
    return even_series_codes_and_odd_bits ^ ODD_BITS;
  }

  simdjson_inline uint64_t follows_nonquote_scalar(const uint64_t nonquote_scalar) noexcept {
    const uint64_t shifted{(nonquote_scalar << 1) | prev_scalar};
    prev_scalar = nonquote_scalar >> 63;
    return shifted;
  }

  simdjson_inline void finish_next() noexcept {
    const uint64_t in_string{prefix_xor(rope_block::quotes) ^ prev_in_string};
    prev_in_string = static_cast<uint64_t>(static_cast<int64_t>(in_string) >> 63);
    rope_block::in_string = in_string;
  }

  uint64_t next_is_escaped{};
  uint64_t prev_in_string{};
  uint64_t prev_scalar{};
};

struct tape_writer_op {
  simdjson_inline static uint32_t extract_index(const uint64_t base, const uint64_t bits) noexcept {
    return static_cast<uint32_t>(static_cast<uint64_t>(trailing_zeroes(bits)) + base);
  }

  simdjson_inline static uint64_t corrected_popcount(const uint64_t bits) noexcept {
    return static_cast<uint64_t>(count_ones(bits));
  }

  simdjson_inline static uint64_t advance(const uint64_t bits) noexcept {
    return clear_lowest_bit(bits);
  }
};

template<uint64_t size> consteval std::array<uint8_t, size> generate_whitespace_array() noexcept {
  constexpr uint8_t values[]{0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0xFFu, 0xFFu, 0x00u, 0x00u, 0xFFu, 0x00u, 0x00u};
  std::array<uint8_t, size> return_values{};
  for (uint64_t x{0}; x < size; ++x) {
    return_values[x] = values[x % 16];
  }
  return return_values;
}

template<uint64_t size> alignas(16) inline constexpr std::array<uint8_t, size> whitespace_array{generate_whitespace_array<size>()};

template<uint64_t size> consteval std::array<uint8_t, size> generate_op_array() noexcept {
  constexpr uint8_t values[]{0xFFu, 0x00u, uint8_t(','), uint8_t(':'), 0x00u, uint8_t('['), uint8_t(']'), uint8_t('{'), uint8_t('}'), 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u};
  std::array<uint8_t, size> return_values{};
  for (uint64_t x{0}; x < size; ++x) {
    return_values[x] = values[x % 16];
  }
  return return_values;
}

template<uint64_t size> alignas(16) inline constexpr std::array<uint8_t, size> op_array{generate_op_array<size>()};

} // namespace jsonifier
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_JSONIFIER_NEON

#endif // SIMDJSON_SRC_GENERIC_JSONIFIER_NEON_STAGE1_H
