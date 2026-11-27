// Code below from Jsonifier's batched-drain architecture:
// https://nihilai-collective.net/batched-drain

#ifndef SIMDJSON_SRC_GENERIC_JSONIFIER_ADD_TAPE_VALUES_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#define SIMDJSON_SRC_GENERIC_JSONIFIER_ADD_TAPE_VALUES_H
#include <generic/jsonifier/avx_stage1.h>
#include <generic/jsonifier/neon_stage1.h>
#endif // SIMDJSON_CONDITIONAL_INCLUDE

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace {
namespace jsonifier {

template<typename index_sequence_type> struct add_tape_values;

#if SIMDJSON_JSONIFIER_AVX512

template<uint64_t... indices> struct add_tape_values<std::integer_sequence<uint64_t, indices...>> {
  static constexpr uint64_t BLOCK_COUNT{sizeof...(indices)};

  simdjson_inline static void impl(const std::array<uint64_t, BLOCK_COUNT> &__restrict bits_arr, const std::array<uint64_t, BLOCK_COUNT> &__restrict cnts,
    write_structural_index_ptr tape, const uint64_t str_idx) noexcept {
    uint64_t offset{0};
    const __m512i compression_mask{_mm512_set_epi32(0x3f3e3d3c, 0x3b3a3938, 0x37363534, 0x33323130, 0x2f2e2d2c, 0x2b2a2928, 0x27262524, 0x23222120, 0x1f1e1d1c,
      0x1b1a1918, 0x17161514, 0x13121110, 0x0f0e0d0c, 0x0b0a0908, 0x07060504, 0x03020100)};
    const __m512i str_idx_vec{_mm512_set1_epi32(static_cast<int32_t>(str_idx))};
    ((drain_lane<indices>(compression_mask, str_idx_vec, bits_arr, cnts, tape + offset), offset += cnts[indices]), ...);
  }

  template<uint64_t index> simdjson_inline static void drain_lane(const __m512i compression_mask, const __m512i str_idx_vec,
    const std::array<uint64_t, BLOCK_COUNT> &__restrict bits_arr, const std::array<uint64_t, BLOCK_COUNT> &__restrict cnts, write_structural_index_ptr tape) noexcept {
    const uint64_t bits{bits_arr[index]};
    if (!bits) [[unlikely]] {
      return;
    }
    const uint64_t count{cnts[index]};
    constexpr int32_t BIT_TOTAL{static_cast<int32_t>(index * 64ull)};
    const __m512i indexes{_mm512_maskz_compress_epi8(bits, compression_mask)};
    const __m512i start_index{_mm512_add_epi32(str_idx_vec, _mm512_set1_epi32(BIT_TOTAL))};
    _mm512_storeu_si512(tape, _mm512_add_epi32(_mm512_cvtepu8_epi32(_mm512_castsi512_si128(indexes)), start_index));
    if (count > 16) {
      _mm512_storeu_si512(tape + 16, _mm512_add_epi32(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 1)), start_index));
      if (count > 32) {
        _mm512_storeu_si512(tape + 32, _mm512_add_epi32(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 2)), start_index));
        if (count > 48) {
          _mm512_storeu_si512(tape + 48, _mm512_add_epi32(_mm512_cvtepu8_epi32(_mm512_extracti32x4_epi32(indexes, 3)), start_index));
        }
      }
    }
  }
};

#else

template<typename index_sequence_type> struct write_indices;

template<uint64_t... offsets> struct write_indices<std::integer_sequence<uint64_t, offsets...>> {
  simdjson_inline static void impl(const uint64_t base, uint64_t &__restrict bits, write_structural_index_ptr tape) noexcept {
    ((tape[offsets] = tape_writer_op::extract_index(base, bits), bits = tape_writer_op::advance(bits)), ...);
  }
};

template<typename index_sequence_type> struct write_indices_stepped;

template<uint64_t... chunks> struct write_indices_stepped<std::integer_sequence<uint64_t, chunks...>> {
  using chunk_writer = write_indices<std::make_integer_sequence<uint64_t, TAPE_STEP>>;

  template<uint64_t start> simdjson_inline static bool chunk(const uint64_t base, uint64_t &__restrict bits, write_structural_index_ptr tape, const uint64_t cnt) noexcept {
    if constexpr (start > 0) {
      if (start < cnt) [[unlikely]] {
        chunk_writer::impl(base, bits, tape + start);
        return true;
      }
      return false;
    } else {
      chunk_writer::impl(base, bits, tape + start);
      return true;
    }
  }

  simdjson_inline static void impl(const uint64_t base, uint64_t &__restrict bits, write_structural_index_ptr tape, const uint64_t cnt) noexcept {
    (chunk<chunks * TAPE_STEP>(base, bits, tape, cnt) && ...);
  }
};

template<uint64_t... indices> struct add_tape_values<std::integer_sequence<uint64_t, indices...>> {
  static constexpr uint64_t BLOCK_COUNT{sizeof...(indices)};
  using lane_writer = write_indices_stepped<std::make_integer_sequence<uint64_t, 64 / TAPE_STEP>>;

  simdjson_inline static void impl(const std::array<uint64_t, BLOCK_COUNT> &__restrict bits_arr, const std::array<uint64_t, BLOCK_COUNT> &__restrict cnts,
    write_structural_index_ptr tape, const uint64_t str_idx) noexcept {
    uint64_t offset{0};
    ((drain_lane<indices>(bits_arr, cnts, tape + offset, str_idx), offset += cnts[indices]), ...);
  }

  template<uint64_t index> simdjson_inline static void drain_lane(const std::array<uint64_t, BLOCK_COUNT> &__restrict bits_arr,
    const std::array<uint64_t, BLOCK_COUNT> &__restrict cnts, write_structural_index_ptr tape, const uint64_t str_idx) noexcept {
    uint64_t bits{bits_arr[index]};
    lane_writer::impl(index * 64ull + str_idx, bits, tape, cnts[index]);
  }
};

#endif

} // namespace jsonifier
} // unnamed namespace
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SRC_GENERIC_JSONIFIER_ADD_TAPE_VALUES_H
