// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#ifndef SIMDJSON_SRC_GENERIC_JSONIFIER_AVX_STAGE1_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSONIFIER_AVX_STAGE1_H
#include <generic/jsonifier/avx.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#if SIMDJSON_JSONIFIER_ANY_AVX

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsonifier {

// The modulo keeps the shifts of the discarded branches below 64 bits, since
// MSVC still evaluates them in a function that is not a template.
constexpr uint64_t register_shift(const uint64_t index) noexcept {
  return (BYTES_PER_REGISTER * index) % 64;
}

struct cmp_eq_op {
  simdjson_inline static uint64_t impl(const simd_array_t lhs, const simd_int_t rhs_broadcast) noexcept {
    uint64_t result{op_cmp_eq(lhs.get<0>(), rhs_broadcast)};
    if constexpr (REGISTERS_PER_BLOCK > 1) {
      result |= op_cmp_eq(lhs.get<1>(), rhs_broadcast) << register_shift(1);
      if constexpr (REGISTERS_PER_BLOCK > 2) {
        result |= op_cmp_eq(lhs.get<2>(), rhs_broadcast) << register_shift(2);
        result |= op_cmp_eq(lhs.get<3>(), rhs_broadcast) << register_shift(3);
      }
    }
    return result;
  }

  simdjson_inline static uint64_t impl(const simd_array_t lhs, const simd_array_t rhs) noexcept {
    uint64_t result{op_cmp_eq(lhs.get<0>(), rhs.get<0>())};
    if constexpr (REGISTERS_PER_BLOCK > 1) {
      result |= op_cmp_eq(lhs.get<1>(), rhs.get<1>()) << register_shift(1);
      if constexpr (REGISTERS_PER_BLOCK > 2) {
        result |= op_cmp_eq(lhs.get<2>(), rhs.get<2>()) << register_shift(2);
        result |= op_cmp_eq(lhs.get<3>(), rhs.get<3>()) << register_shift(3);
      }
    }
    return result;
  }
};

struct ws_collector {
  simdjson_inline static uint64_t impl(const simd_array_t in01, const simd_int_t whitespace_table) noexcept {
    simd_array_t ws_shuffle;
    ws_shuffle.set<0>(op_shuffle(whitespace_table, in01.get<0>()));
    if constexpr (REGISTERS_PER_BLOCK > 1) {
      ws_shuffle.set<1>(op_shuffle(whitespace_table, in01.get<1>()));
      if constexpr (REGISTERS_PER_BLOCK > 2) {
        ws_shuffle.set<2>(op_shuffle(whitespace_table, in01.get<2>()));
        ws_shuffle.set<3>(op_shuffle(whitespace_table, in01.get<3>()));
      }
    }
    return cmp_eq_op::impl(in01, ws_shuffle);
  }
};

struct op_collector {
  simdjson_inline static uint64_t impl(const simd_array_t in01, const simd_int_t op_table, const simd_int_t space_mask) noexcept {
    simd_array_t or_lhs;
    simd_array_t shuffle_rhs;
    or_lhs.set<0>(op_or(in01.get<0>(), space_mask));
    shuffle_rhs.set<0>(op_shuffle(op_table, in01.get<0>()));
    if constexpr (REGISTERS_PER_BLOCK > 1) {
      or_lhs.set<1>(op_or(in01.get<1>(), space_mask));
      shuffle_rhs.set<1>(op_shuffle(op_table, in01.get<1>()));
      if constexpr (REGISTERS_PER_BLOCK > 2) {
        or_lhs.set<2>(op_or(in01.get<2>(), space_mask));
        shuffle_rhs.set<2>(op_shuffle(op_table, in01.get<2>()));
        or_lhs.set<3>(op_or(in01.get<3>(), space_mask));
        shuffle_rhs.set<3>(op_shuffle(op_table, in01.get<3>()));
      }
    }
    return cmp_eq_op::impl(or_lhs, shuffle_rhs);
  }
};

template<uint64_t register_bytes, uint64_t register_count> struct pod_cmp_eq_op {
  using simd_type = simd_register_t<register_bytes>;
  using simd_array_type = pod_simd_array_t<register_count, register_bytes>;

  simdjson_inline static uint64_t impl(const simd_array_type lhs, const simd_array_type rhs) noexcept {
    uint64_t result{op_cmp_eq(lhs.template get<0>(), rhs.template get<0>())};
    if constexpr (register_count > 1) {
      result |= op_cmp_eq(lhs.template get<1>(), rhs.template get<1>()) << (register_bytes * 1);
      if constexpr (register_count > 2) {
        result |= op_cmp_eq(lhs.template get<2>(), rhs.template get<2>()) << (register_bytes * 2);
        result |= op_cmp_eq(lhs.template get<3>(), rhs.template get<3>()) << (register_bytes * 3);
      }
    }
    return result;
  }

  simdjson_inline static uint64_t impl(const simd_array_type lhs, const simd_type rhs_broadcast) noexcept {
    uint64_t result{op_cmp_eq(lhs.template get<0>(), rhs_broadcast)};
    if constexpr (register_count > 1) {
      result |= op_cmp_eq(lhs.template get<1>(), rhs_broadcast) << (register_bytes * 1);
      if constexpr (register_count > 2) {
        result |= op_cmp_eq(lhs.template get<2>(), rhs_broadcast) << (register_bytes * 2);
        result |= op_cmp_eq(lhs.template get<3>(), rhs_broadcast) << (register_bytes * 3);
      }
    }
    return result;
  }

  static_assert(register_count >= 1 && register_count * register_bytes <= BYTES_PER_BLOCK, "A pod block must fit in 64 bytes.");
};

template<uint64_t register_bytes, uint64_t register_count> struct pod_ws_collector {
  using simd_type = simd_register_t<register_bytes>;
  using simd_array_type = pod_simd_array_t<register_count, register_bytes>;

  simdjson_inline static uint64_t impl(const simd_array_type in01, const simd_type whitespace_table) noexcept {
    simd_array_type ws_shuffle;
    ws_shuffle.template set<0>(op_shuffle(whitespace_table, in01.template get<0>()));
    if constexpr (register_count > 1) {
      ws_shuffle.template set<1>(op_shuffle(whitespace_table, in01.template get<1>()));
      if constexpr (register_count > 2) {
        ws_shuffle.template set<2>(op_shuffle(whitespace_table, in01.template get<2>()));
        ws_shuffle.template set<3>(op_shuffle(whitespace_table, in01.template get<3>()));
      }
    }
    return pod_cmp_eq_op<register_bytes, register_count>::impl(in01, ws_shuffle);
  }
};

template<uint64_t register_bytes, uint64_t register_count> struct scalar_op_collector {
  using simd_type = simd_register_t<register_bytes>;
  using simd_array_type = pod_simd_array_t<register_count, register_bytes>;

  simdjson_inline static uint64_t impl(const simd_array_type in01, const simd_type op_table, const simd_type space_mask) noexcept {
    simd_array_type or_lhs;
    simd_array_type shuffle_rhs;
    or_lhs.template set<0>(op_or(in01.template get<0>(), space_mask));
    shuffle_rhs.template set<0>(op_shuffle(op_table, in01.template get<0>()));
    if constexpr (register_count > 1) {
      or_lhs.template set<1>(op_or(in01.template get<1>(), space_mask));
      shuffle_rhs.template set<1>(op_shuffle(op_table, in01.template get<1>()));
      if constexpr (register_count > 2) {
        or_lhs.template set<2>(op_or(in01.template get<2>(), space_mask));
        shuffle_rhs.template set<2>(op_shuffle(op_table, in01.template get<2>()));
        or_lhs.template set<3>(op_or(in01.template get<3>(), space_mask));
        shuffle_rhs.template set<3>(op_shuffle(op_table, in01.template get<3>()));
      }
    }
    return pod_cmp_eq_op<register_bytes, register_count>::impl(or_lhs, shuffle_rhs);
  }
};

template<typename rope_block> struct rope_detector : rope_block {
  template<uint64_t register_bytes, uint64_t register_count> simdjson_inline void next_scalar(const pod_simd_array_t<register_count, register_bytes> in01,
    const simd_register_t<register_bytes> bs_register, const simd_register_t<register_bytes> quote_register) noexcept {
    const uint64_t escaped{next_escape_and_terminal_code(pod_cmp_eq_op<register_bytes, register_count>::impl(in01, bs_register))};
    const uint64_t quotes{pod_cmp_eq_op<register_bytes, register_count>::impl(in01, quote_register) & ~escaped};
    rope_block::escaped = escaped;
    rope_block::quotes = quotes;
    finish_next();
  }

  simdjson_inline void next(const simd_array_t in01, const simd_int_t bs_register, const simd_int_t quote_register) noexcept {
    const uint64_t escaped{next_escape_and_terminal_code(cmp_eq_op::impl(in01, bs_register))};
    uint64_t raw_quotes{cmp_eq_op::impl(in01, quote_register)};
#if SIMDJSON_JSONIFIER_GCC
    __asm__("" : "+r"(raw_quotes));
#endif
    const uint64_t quotes{raw_quotes & ~escaped};
    rope_block::escaped = escaped;
    rope_block::quotes = quotes;
    finish_next();
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
  constexpr uint8_t values[]{0x20u, 0x64u, 0x64u, 0x64u, 0x11u, 0x64u, 0x71u, 0x02u, 0x64u, uint8_t('\t'), uint8_t('\n'), 0x70u, 0x64u, uint8_t('\r'), 0x64u, 0x64u};
  std::array<uint8_t, size> return_values{};
  for (uint64_t x{0}; x < size; ++x) {
    return_values[x] = values[x % 16];
  }
  return return_values;
}

template<uint64_t size> alignas(64) inline constexpr std::array<uint8_t, size> whitespace_array{generate_whitespace_array<size>()};

template<uint64_t size> consteval std::array<uint8_t, size> generate_op_array() noexcept {
  constexpr uint8_t values[]{0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x00u, uint8_t(':'), uint8_t('{'), uint8_t(','), uint8_t('}'), 0x00u, 0x00u};
  std::array<uint8_t, size> return_values{};
  for (uint64_t x{0}; x < size; ++x) {
    return_values[x] = values[x % 16];
  }
  return return_values;
}

template<uint64_t size> alignas(64) inline constexpr std::array<uint8_t, size> op_array{generate_op_array<size>()};

} // namespace jsonifier
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_JSONIFIER_ANY_AVX

#endif // SIMDJSON_SRC_GENERIC_JSONIFIER_AVX_STAGE1_H
