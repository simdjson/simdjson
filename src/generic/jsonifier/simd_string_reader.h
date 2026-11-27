// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#ifndef SIMDJSON_SRC_GENERIC_JSONIFIER_SIMD_STRING_READER_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSONIFIER_SIMD_STRING_READER_H
#include <generic/jsonifier/add_tape_values.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsonifier {

template<typename index_sequence_type> struct tail_block_steps;

template<uint64_t... indices> struct tail_block_steps<std::integer_sequence<uint64_t, indices...>> {
  template<uint64_t offset> simdjson_inline static void step(write_buffer_ptr dst, read_buffer_ptr src, const uint64_t remaining, const simd_int_t lanes,
    const simd_int_t spaces) noexcept {
    const uint64_t lanes_left{remaining > offset ? (remaining - offset < BYTES_PER_REGISTER ? remaining - offset : BYTES_PER_REGISTER) : 0};
    const simd_int_t keep{op_cmp_lt_raw(lanes, simd_loader<BYTES_PER_REGISTER>::splat(static_cast<uint8_t>(lanes_left)))};
    const simd_int_t data{simd_loader<BYTES_PER_REGISTER>::load_u(src + offset)};
    store_u(op_xor(spaces, op_and(op_xor(data, spaces), keep)), dst + offset);
  }

  simdjson_inline static void impl(write_buffer_ptr dst, read_buffer_ptr src, const uint64_t remaining, const simd_int_t lanes, const simd_int_t spaces) noexcept {
    (step<indices * BYTES_PER_REGISTER>(dst, src, remaining, lanes, spaces), ...);
  }
};

alignas(64) inline constexpr uint8_t LANE_INDICES[64]{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30,
  31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63};

struct tail_block_builder {
  static constexpr uint64_t PAGE_SIZE{4096};

  template<uint64_t padded_bytes> simdjson_inline static void build_in_place(write_buffer_ptr dst, read_buffer_ptr src, const uint64_t remaining) noexcept {
    const simd_int_t spaces{simd_loader<BYTES_PER_REGISTER>::splat(0x20)};
    const simd_int_t lanes{simd_loader<BYTES_PER_REGISTER>::load(LANE_INDICES)};
    tail_block_steps<std::make_integer_sequence<uint64_t, (padded_bytes + BYTES_PER_REGISTER - 1) / BYTES_PER_REGISTER>>::impl(dst, src, remaining, lanes, spaces);
  }

  template<uint64_t padded_bytes> simdjson_inline static void build_by_copy(write_buffer_ptr dst, read_buffer_ptr src, const uint64_t remaining) noexcept {
    std::memset(dst + remaining, 0x20, padded_bytes - remaining);
    std::memcpy(dst, src, remaining);
  }

  template<uint64_t padded_bytes> simdjson_inline static void impl(write_buffer_ptr dst, read_buffer_ptr src, const uint64_t remaining) noexcept {
    if (read_stays_in_page<padded_bytes>(src, remaining)) [[likely]] {
      build_in_place<padded_bytes>(dst, src, remaining);
    } else {
      build_by_copy<padded_bytes>(dst, src, remaining);
    }
  }

  template<uint64_t padded_bytes> simdjson_inline static bool read_stays_in_page(read_buffer_ptr src, const uint64_t remaining) noexcept {
    if constexpr (SIMDJSON_JSONIFIER_ASAN) {
      return false;
    } else {
      const uint64_t last_valid{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(src)) + remaining - 1};
      const uint64_t last_read{static_cast<uint64_t>(reinterpret_cast<uintptr_t>(src)) + padded_bytes - 1};
      return (last_valid ^ last_read) < PAGE_SIZE;
    }
  }
};

struct string_block_reader {
  template<uint64_t padded_bytes> simdjson_inline const uint8_t *get_remainder() noexcept {
    tail_block_builder::template impl<padded_bytes>(+block, in_string + index, length - index);
    return +block;
  }

  simdjson_inline void reset(read_buffer_ptr string_new, const uint64_t length_new) noexcept {
    in_string = string_new;
    length = length_new;
    index = 0;
  }

  simdjson_inline uint64_t get_remainder_bytes() const noexcept {
    return length - index;
  }

  alignas(64) uint8_t block[BYTES_PER_BLOCK]{};
  const uint8_t *in_string{};
  uint64_t length{};
  uint64_t index{};
};

struct string_block_reader_locals {
  template<uint64_t padded_bytes> simdjson_inline const uint8_t *get_remainder() noexcept {
    tail_block_builder::template impl<padded_bytes>(+block, in_string + index, length - index);
    return +block;
  }

  simdjson_inline void reset(read_buffer_ptr string_new, const uint64_t length_new) noexcept {
    in_string = string_new;
    length = length_new;
    index = 0;
  }

  alignas(64) uint8_t block[BYTES_PER_BLOCK]{};
  const uint8_t *in_string{};
  uint64_t length{};
  uint64_t index{};
};

struct rope_block {
  simdjson_inline uint64_t non_quote_outside_string(const uint64_t mask) const noexcept {
    return mask & ~in_string;
  }

  simdjson_inline uint64_t string_tail() const noexcept {
    return in_string ^ quotes;
  }

  uint64_t in_string{};
  uint64_t escaped{};
  uint64_t quotes{};
};

using rope_type = rope_detector<rope_block>;

template<bool minified, uint64_t register_bytes, uint64_t register_count> struct narrow_tail_block {
  using simd_type = simd_register_t<register_bytes>;

  simdjson_inline static uint64_t impl(rope_type &__restrict rope, read_buffer_ptr block_ptr, write_structural_index_ptr tape_ptr, const uint64_t block_base_index) noexcept {
    pod_simd_array_t<register_count, register_bytes> in_vals;
    in_vals.template set<0>(simd_loader<register_bytes>::load_u(block_ptr));
    if constexpr (register_count > 1) {
      in_vals.template set<1>(simd_loader<register_bytes>::load_u(block_ptr + register_bytes));
    }
    rope.template next_scalar<register_bytes, register_count>(in_vals, simd_loader<register_bytes>::splat('\\'), simd_loader<register_bytes>::splat('"'));
    const uint64_t op{scalar_op_collector<register_bytes, register_count>::impl(in_vals, simd_loader<register_bytes>::load(op_array<register_bytes>.data()),
      simd_loader<register_bytes>::splat(0x20))};
    uint64_t whitespace{};
    if constexpr (!minified) {
      whitespace = pod_ws_collector<register_bytes, register_count>::impl(in_vals, simd_loader<register_bytes>::load(whitespace_array<register_bytes>.data()));
    }
    const uint64_t scalar{~(op | whitespace | rope.quotes)};
    const uint64_t follows{rope.follows_nonquote_scalar(scalar)};
    std::array<uint64_t, 1> bits_arr;
    std::array<uint64_t, 1> cnts_arr;
    bits_arr[0] = (op | rope.quotes | (scalar & ~follows)) & ~rope.string_tail();
    cnts_arr[0] = tape_writer_op::corrected_popcount(bits_arr[0]);
    add_tape_values<std::make_integer_sequence<uint64_t, 1>>::impl(bits_arr, cnts_arr, tape_ptr, block_base_index);
    return cnts_arr[0];
  }
};

template<bool minified> struct narrow_tail_dispatch {
  simdjson_inline static bool impl(rope_type &__restrict rope, read_buffer_ptr block_ptr, write_structural_index_ptr tape_ptr, const uint64_t block_base_index,
    const uint64_t remaining, uint64_t &count) noexcept {
#if SIMDJSON_JSONIFIER_ANY_AVX
    if (remaining <= 16) {
      count = narrow_tail_block<minified, 16, 1>::impl(rope, block_ptr, tape_ptr, block_base_index);
      return true;
    }
    if constexpr (BYTES_PER_REGISTER >= 32) {
      if (remaining <= 32) {
        count = narrow_tail_block<minified, 32, 1>::impl(rope, block_ptr, tape_ptr, block_base_index);
        return true;
      }
    }
#elif SIMDJSON_JSONIFIER_NEON
    if (remaining <= 16) {
      count = narrow_tail_block<minified, 16, 1>::impl(rope, block_ptr, tape_ptr, block_base_index);
      return true;
    }
    if (remaining <= 32) {
      count = narrow_tail_block<minified, 16, 2>::impl(rope, block_ptr, tape_ptr, block_base_index);
      return true;
    }
#endif
    return false;
  }
};

struct simd_string_reader_members : rope_type, string_block_reader {
  template<bool minified> simdjson_inline void reset(read_buffer_ptr root_iter, const uint64_t string_length, write_structural_index_ptr tape_new) noexcept {
    tape = tape_new;
    tape_count = 0;
    string_block_reader::reset(root_iter, string_length);
    rope_type::prev_in_string = 0;
    rope_type::prev_scalar = 0;
    rope_type::next_is_escaped = 0;

    const simd_int_t bs_register{simd_loader<BYTES_PER_REGISTER>::splat('\\')};
    const simd_int_t quote_register{simd_loader<BYTES_PER_REGISTER>::splat('"')};
    const simd_int_t op_table{simd_loader<BYTES_PER_REGISTER>::load(op_array<BYTES_PER_REGISTER>.data())};
    const simd_int_t space_mask{simd_loader<BYTES_PER_REGISTER>::splat(0x20)};

    if constexpr (minified) {
      reset_impl<minified>(bs_register, quote_register, op_table, space_mask);
    } else {
      const simd_int_t whitespace_table{simd_loader<BYTES_PER_REGISTER>::load(whitespace_array<BYTES_PER_REGISTER>.data())};
      reset_impl<minified>(bs_register, quote_register, op_table, space_mask, whitespace_table);
    }
    ends_in_string = rope_type::prev_in_string != 0;
  }

  write_structural_index_ptr tape{};
  uint64_t tape_count{};
  bool ends_in_string{};

protected:
  template<uint64_t block_count, typename... simd_types> simdjson_inline void process_blocks(read_buffer_ptr block_ptr, const uint64_t step_base_index,
    const simd_int_t bs_register, const simd_int_t quote_register, const simd_int_t op_table, const simd_int_t space_mask, const simd_types... args) noexcept {
    std::array<uint64_t, block_count> bits_arr;
    std::array<uint64_t, block_count> cnts_arr;
    process_blocks_impl<0>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
    if constexpr (block_count > 1) {
      process_blocks_impl<1>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
      if constexpr (block_count > 2) {
        process_blocks_impl<2>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
        process_blocks_impl<3>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
        if constexpr (block_count > 4) {
          process_blocks_impl<4>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
          process_blocks_impl<5>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
          process_blocks_impl<6>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
          process_blocks_impl<7>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
        }
      }
    }

    add_tape_values<std::make_integer_sequence<uint64_t, block_count>>::impl(bits_arr, cnts_arr, tape + tape_count, step_base_index);

    tape_count += cnts_arr[0];
    if constexpr (block_count > 1) {
      tape_count += cnts_arr[1];
      if constexpr (block_count > 2) {
        tape_count += cnts_arr[2];
        tape_count += cnts_arr[3];
        if constexpr (block_count > 4) {
          tape_count += cnts_arr[4];
          tape_count += cnts_arr[5];
          tape_count += cnts_arr[6];
          tape_count += cnts_arr[7];
        }
      }
    }
  }

  template<bool minified, typename... simd_types> simdjson_inline void reset_impl(const simd_int_t bs_register, const simd_int_t quote_register, const simd_int_t op_table,
    const simd_int_t space_mask, const simd_types... args) noexcept {
    drain_cascade<BLOCKS_PER_STEP>(bs_register, quote_register, op_table, space_mask, args...);

    if (const uint64_t remaining = string_block_reader::get_remainder_bytes(); remaining != 0) {
      read_buffer_ptr tail_ptr{string_block_reader::template get_remainder<BYTES_PER_BLOCK>()};
      uint64_t narrow_count{};
      if (narrow_tail_dispatch<minified>::impl(*this, tail_ptr, tape + tape_count, string_block_reader::index, remaining, narrow_count)) {
        tape_count += narrow_count;
      } else {
        process_tail_block(tail_ptr, string_block_reader::index, bs_register, quote_register, op_table, space_mask, args...);
      }
      while (tape_count > 0 && tape[tape_count - 1] >= string_block_reader::length) {
        --tape_count;
      }
    }
  }

  template<uint64_t block_count, typename... simd_types> simdjson_inline void drain_cascade(const simd_int_t bs_register, const simd_int_t quote_register,
    const simd_int_t op_table, const simd_int_t space_mask, const simd_types... args) noexcept {
    while (string_block_reader::get_remainder_bytes() >= block_count * 64) {
      const uint64_t step_base_index{string_block_reader::index};
      process_blocks<block_count>(string_block_reader::in_string + step_base_index, step_base_index, bs_register, quote_register, op_table, space_mask, args...);
      string_block_reader::index += block_count * 64;
    }
    if constexpr (block_count > 1) {
      drain_cascade<block_count / 2>(bs_register, quote_register, op_table, space_mask, args...);
    }
  }

  template<uint64_t current_block, size_t lane_count, typename... simd_types> simdjson_inline void process_blocks_impl(std::array<uint64_t, lane_count> &bits_arr,
    std::array<uint64_t, lane_count> &cnts_arr, read_buffer_ptr block_ptr, const simd_int_t bs_register, const simd_int_t quote_register, const simd_int_t op_table,
    const simd_int_t space_mask, const simd_types... args) noexcept {
    simd_array_t in_vals;
    in_vals.template set<0>(simd_loader<BYTES_PER_REGISTER>::load_u(block_ptr + current_block * 64));
    if constexpr (REGISTERS_PER_BLOCK > 1) {
      in_vals.template set<1>(simd_loader<BYTES_PER_REGISTER>::load_u(block_ptr + current_block * 64 + BYTES_PER_REGISTER * 1));
      if constexpr (REGISTERS_PER_BLOCK > 2) {
        in_vals.template set<2>(simd_loader<BYTES_PER_REGISTER>::load_u(block_ptr + current_block * 64 + BYTES_PER_REGISTER * 2));
        in_vals.template set<3>(simd_loader<BYTES_PER_REGISTER>::load_u(block_ptr + current_block * 64 + BYTES_PER_REGISTER * 3));
      }
    }
    rope_type::next(in_vals, bs_register, quote_register);
    const uint64_t structurals{get_structurals(in_vals, op_table, space_mask, args...) & ~rope_type::string_tail()};
    bits_arr[current_block] = structurals;
    cnts_arr[current_block] = tape_writer_op::corrected_popcount(structurals);
  }

  template<typename... simd_types> simdjson_inline void process_tail_block(read_buffer_ptr block_ptr, const uint64_t block_base_index, const simd_int_t bs_register,
    const simd_int_t quote_register, const simd_int_t op_table, const simd_int_t space_mask, const simd_types... args) noexcept {
    std::array<uint64_t, 1> bits_arr;
    std::array<uint64_t, 1> cnts_arr;
    process_blocks_impl<0>(bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
    add_tape_values<std::make_integer_sequence<uint64_t, 1>>::impl(bits_arr, cnts_arr, tape + tape_count, block_base_index);
    tape_count += cnts_arr[0];
  }

  simdjson_inline uint64_t get_structurals(const simd_array_t in01, const simd_int_t op_table, const simd_int_t space_mask) noexcept {
    const uint64_t op{op_collector::impl(in01, op_table, space_mask)};
    const uint64_t scalar{~(op | rope_type::quotes)};
    const uint64_t follows{rope_type::follows_nonquote_scalar(scalar)};
    return op | rope_type::quotes | (scalar & ~follows);
  }

  simdjson_inline uint64_t get_structurals(const simd_array_t in01, const simd_int_t op_table, const simd_int_t space_mask, const simd_int_t whitespace_table) noexcept {
#if SIMDJSON_JSONIFIER_NEON
    const uint64x2_t ws_op{ws_op_collector::impl(in01, op_table, space_mask, whitespace_table)};
    const uint64_t whitespace{vgetq_lane_u64(ws_op, 0)};
    const uint64_t op{vgetq_lane_u64(ws_op, 1)};
#else
    const uint64_t whitespace{ws_collector::impl(in01, whitespace_table)};
    const uint64_t op{op_collector::impl(in01, op_table, space_mask)};
#endif
    const uint64_t scalar{~(op | whitespace | rope_type::quotes)};
    const uint64_t follows{rope_type::follows_nonquote_scalar(scalar)};
    return op | rope_type::quotes | (scalar & ~follows);
  }
};

struct simd_string_reader_locals : string_block_reader_locals {
#if SIMDJSON_JSONIFIER_CLANG && SIMDJSON_JSONIFIER_NEON
  template<uint64_t offset> simdjson_inline static void collect_counts_quad(const std::array<uint64_t, BLOCKS_PER_STEP> &__restrict bits_arr,
    std::array<uint64_t, BLOCKS_PER_STEP> &__restrict cnts_arr) noexcept {
    const uint8x16_t counts01{vcntq_u8(vreinterpretq_u8_u64(vcombine_u64(vcreate_u64(bits_arr[offset]), vcreate_u64(bits_arr[offset + 1]))))};
    const uint8x16_t counts23{vcntq_u8(vreinterpretq_u8_u64(vcombine_u64(vcreate_u64(bits_arr[offset + 2]), vcreate_u64(bits_arr[offset + 3]))))};
    uint8x16_t sums{vpaddq_u8(counts01, counts23)};
    sums = vpaddq_u8(sums, sums);
    sums = vpaddq_u8(sums, sums);
    const uint32_t packed{vgetq_lane_u32(vreinterpretq_u32_u8(sums), 0)};
    cnts_arr[offset] = packed & 0xFFu;
    cnts_arr[offset + 1] = (packed >> 8) & 0xFFu;
    cnts_arr[offset + 2] = (packed >> 16) & 0xFFu;
    cnts_arr[offset + 3] = packed >> 24;
  }

  simdjson_inline static void collect_counts(const std::array<uint64_t, BLOCKS_PER_STEP> &__restrict bits_arr, std::array<uint64_t, BLOCKS_PER_STEP> &__restrict cnts_arr) noexcept {
    collect_counts_quad<0>(bits_arr, cnts_arr);
    if constexpr (BLOCKS_PER_STEP > 4) {
      collect_counts_quad<4>(bits_arr, cnts_arr);
    }
  }

  static_assert(BLOCKS_PER_STEP == 4 || BLOCKS_PER_STEP == 8, "The NEON count collector packs four lanes at a time.");
#endif

  template<uint64_t current_block, size_t lane_count, typename... simd_types> simdjson_inline static void process_blocks_impl(rope_type &__restrict rope,
    std::array<uint64_t, lane_count> &__restrict bits_arr, std::array<uint64_t, lane_count> &__restrict cnts_arr, read_buffer_ptr block_ptr, const simd_int_t bs_register,
    const simd_int_t quote_register, const simd_int_t op_table, const simd_int_t space_mask, const simd_types... args) noexcept {
    simd_array_t in_vals;
    in_vals.template set<0>(simd_loader<BYTES_PER_REGISTER>::load_u(block_ptr + current_block * 64));
    if constexpr (REGISTERS_PER_BLOCK > 1) {
      in_vals.template set<1>(simd_loader<BYTES_PER_REGISTER>::load_u(block_ptr + current_block * 64 + BYTES_PER_REGISTER * 1));
      if constexpr (REGISTERS_PER_BLOCK > 2) {
        in_vals.template set<2>(simd_loader<BYTES_PER_REGISTER>::load_u(block_ptr + current_block * 64 + BYTES_PER_REGISTER * 2));
        in_vals.template set<3>(simd_loader<BYTES_PER_REGISTER>::load_u(block_ptr + current_block * 64 + BYTES_PER_REGISTER * 3));
      }
    }
    rope.next(in_vals, bs_register, quote_register);
    const uint64_t structurals{get_structurals(rope, in_vals, op_table, space_mask, args...) & ~rope.string_tail()};
    bits_arr[current_block] = structurals;
#if SIMDJSON_JSONIFIER_CLANG && SIMDJSON_JSONIFIER_NEON
    if constexpr (lane_count != BLOCKS_PER_STEP) {
      cnts_arr[current_block] = tape_writer_op::corrected_popcount(structurals);
    }
#else
    cnts_arr[current_block] = tape_writer_op::corrected_popcount(structurals);
#endif
  }

  template<bool minified, typename... simd_types> simdjson_inline void reset_impl(const simd_int_t bs_register, const simd_int_t quote_register, const simd_int_t op_table,
    const simd_int_t space_mask, const simd_types... args) noexcept {
    rope_type rope{};
    write_structural_index_ptr tape_local{tape};
    read_buffer_ptr src_ptr{string_block_reader_locals::in_string};
    const uint64_t length_local{string_block_reader_locals::length};
    uint64_t tape_count_local{0};
    uint64_t index_local{0};

    drain_cascade<BLOCKS_PER_STEP>(rope, src_ptr, tape_local, tape_count_local, index_local, length_local, bs_register, quote_register, op_table, space_mask, args...);
    string_block_reader_locals::index = index_local;

    if (const uint64_t remaining = length_local - index_local; remaining != 0) {
      read_buffer_ptr tail_ptr{string_block_reader_locals::template get_remainder<BYTES_PER_BLOCK>()};
      uint64_t narrow_count{};
      if (narrow_tail_dispatch<minified>::impl(rope, tail_ptr, tape_local + tape_count_local, index_local, remaining, narrow_count)) {
        tape_count_local += narrow_count;
      } else {
        tape_count_local += process_tail_block(rope, tail_ptr, tape_local + tape_count_local, index_local, bs_register, quote_register, op_table, space_mask, args...);
      }
      while (tape_count_local > 0 && tape_local[tape_count_local - 1] >= length_local) {
        --tape_count_local;
      }
    }
    tape_count = tape_count_local;
    ends_in_string = rope.prev_in_string != 0;
  }

  template<uint64_t block_count, typename... simd_types> simdjson_inline static void drain_cascade(rope_type &__restrict rope, read_buffer_ptr src_ptr,
    write_structural_index_ptr tape_ptr, uint64_t &tape_count_local, uint64_t &index_local, const uint64_t length_local, const simd_int_t bs_register,
    const simd_int_t quote_register, const simd_int_t op_table, const simd_int_t space_mask, const simd_types... args) noexcept {
    while (length_local - index_local >= block_count * 64) {
      tape_count_local +=
        process_blocks<block_count>(rope, src_ptr + index_local, tape_ptr + tape_count_local, index_local, bs_register, quote_register, op_table, space_mask, args...);
      index_local += block_count * 64;
    }
    if constexpr (block_count > 1) {
      drain_cascade<block_count / 2>(rope, src_ptr, tape_ptr, tape_count_local, index_local, length_local, bs_register, quote_register, op_table, space_mask, args...);
    }
  }

  template<typename... simd_types> simdjson_inline static uint64_t process_tail_block(rope_type &__restrict rope, read_buffer_ptr block_ptr, write_structural_index_ptr tape_ptr,
    const uint64_t block_base_index, const simd_int_t bs_register, const simd_int_t quote_register, const simd_int_t op_table, const simd_int_t space_mask,
    const simd_types... args) noexcept {
    std::array<uint64_t, 1> bits_arr;
    std::array<uint64_t, 1> cnts_arr;
    process_blocks_impl<0>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
    add_tape_values<std::make_integer_sequence<uint64_t, 1>>::impl(bits_arr, cnts_arr, tape_ptr, block_base_index);
    return cnts_arr[0];
  }

  template<uint64_t block_count, typename... simd_types> simdjson_inline static uint64_t process_blocks(rope_type &__restrict rope, read_buffer_ptr block_ptr,
    write_structural_index_ptr tape_ptr, const uint64_t step_base_index, const simd_int_t bs_register, const simd_int_t quote_register, const simd_int_t op_table,
    const simd_int_t space_mask, const simd_types... args) noexcept {
    std::array<uint64_t, block_count> bits_arr;
    std::array<uint64_t, block_count> cnts_arr;
    process_blocks_impl<0>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
    if constexpr (block_count > 1) {
      process_blocks_impl<1>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
      if constexpr (block_count > 2) {
        process_blocks_impl<2>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
        process_blocks_impl<3>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
        if constexpr (block_count > 4) {
          process_blocks_impl<4>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
          process_blocks_impl<5>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
          process_blocks_impl<6>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
          process_blocks_impl<7>(rope, bits_arr, cnts_arr, block_ptr, bs_register, quote_register, op_table, space_mask, args...);
        }
      }
    }
#if SIMDJSON_JSONIFIER_CLANG && SIMDJSON_JSONIFIER_NEON
    if constexpr (block_count == BLOCKS_PER_STEP) {
      collect_counts(bits_arr, cnts_arr);
    }
#endif

    add_tape_values<std::make_integer_sequence<uint64_t, block_count>>::impl(bits_arr, cnts_arr, tape_ptr, step_base_index);

    uint64_t step_count{cnts_arr[0]};
    if constexpr (block_count > 1) {
      step_count += cnts_arr[1];
      if constexpr (block_count > 2) {
        step_count += cnts_arr[2];
        step_count += cnts_arr[3];
        if constexpr (block_count > 4) {
          step_count += cnts_arr[4];
          step_count += cnts_arr[5];
          step_count += cnts_arr[6];
          step_count += cnts_arr[7];
        }
      }
    }
    return step_count;
  }

  template<bool minified> simdjson_inline void reset(read_buffer_ptr root_iter, const uint64_t string_length, write_structural_index_ptr tape_new) noexcept {
    tape = tape_new;
    string_block_reader_locals::reset(root_iter, string_length);

    const simd_int_t bs_register{simd_loader<BYTES_PER_REGISTER>::splat('\\')};
    const simd_int_t quote_register{simd_loader<BYTES_PER_REGISTER>::splat('"')};
    const simd_int_t op_table{simd_loader<BYTES_PER_REGISTER>::load(op_array<BYTES_PER_REGISTER>.data())};
    const simd_int_t space_mask{simd_loader<BYTES_PER_REGISTER>::splat(0x20)};

    if constexpr (minified) {
      reset_impl<minified>(bs_register, quote_register, op_table, space_mask);
    } else {
      const simd_int_t whitespace_table{simd_loader<BYTES_PER_REGISTER>::load(whitespace_array<BYTES_PER_REGISTER>.data())};
      reset_impl<minified>(bs_register, quote_register, op_table, space_mask, whitespace_table);
    }
  }

  simdjson_inline static uint64_t get_structurals(rope_type &__restrict rope, const simd_array_t in01, const simd_int_t op_table, const simd_int_t space_mask,
    const simd_int_t whitespace_table) noexcept {
#if SIMDJSON_JSONIFIER_NEON
    const uint64x2_t ws_op{ws_op_collector::impl(in01, op_table, space_mask, whitespace_table)};
    const uint64_t whitespace{vgetq_lane_u64(ws_op, 0)};
    const uint64_t op{vgetq_lane_u64(ws_op, 1)};
#else
    const uint64_t whitespace{ws_collector::impl(in01, whitespace_table)};
    const uint64_t op{op_collector::impl(in01, op_table, space_mask)};
#endif
    const uint64_t quotes{rope.quotes};
    const uint64_t scalar{~(op | whitespace | quotes)};
    const uint64_t follows{rope.follows_nonquote_scalar(scalar)};
    return op | quotes | (scalar & ~follows);
  }

  simdjson_inline static uint64_t get_structurals(rope_type &__restrict rope, const simd_array_t in01, const simd_int_t op_table, const simd_int_t space_mask) noexcept {
    const uint64_t op{op_collector::impl(in01, op_table, space_mask)};
    const uint64_t quotes{rope.quotes};
    const uint64_t scalar{~(op | quotes)};
    const uint64_t follows{rope.follows_nonquote_scalar(scalar)};
    return op | quotes | (scalar & ~follows);
  }

  write_structural_index_ptr tape{};
  uint64_t tape_count{};
  bool ends_in_string{};
};

#if SIMDJSON_JSONIFIER_CLANG
using simd_string_reader = simd_string_reader_members;
#else
using simd_string_reader = simd_string_reader_locals;
#endif

} // namespace jsonifier
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_JSONIFIER_SIMD_STRING_READER_H
