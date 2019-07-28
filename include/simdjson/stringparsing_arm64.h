#ifndef SIMDJSON_STRINGPARSING_ARM64_H
#define SIMDJSON_STRINGPARSING_ARM64_H

#include "simdjson/stringparsing.h"
#include "simdjson/stringparsing_macros.h"

#ifdef IS_ARM64
namespace simdjson {
template<> really_inline
parse_string_helper find_bs_bits_and_quote_bits<architecture::arm64> (const uint8_t *src, uint8_t *dst) {
    // this can read up to 31 bytes beyond the buffer size, but we require 
    // SIMDJSON_PADDING of padding
    static_assert(2 * sizeof(uint8x16_t) - 1 <= SIMDJSON_PADDING);
    uint8x16_t v0 = vld1q_u8(src);
    uint8x16_t v1 = vld1q_u8(src+16);
    vst1q_u8(dst, v0);
    vst1q_u8(dst+16, v1);
    
    uint8x16_t bs_mask = vmovq_n_u8('\\');
    uint8x16_t qt_mask = vmovq_n_u8('"');
    const uint8x16_t bitmask = { 0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
                                 0x01, 0x02, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
    uint8x16_t cmp_bs_0 = vceqq_u8(v0, bs_mask);
    uint8x16_t cmp_bs_1 = vceqq_u8(v1, bs_mask);
    uint8x16_t cmp_qt_0 = vceqq_u8(v0, qt_mask);
    uint8x16_t cmp_qt_1 = vceqq_u8(v1, qt_mask);
    
    cmp_bs_0 = vandq_u8(cmp_bs_0, bitmask);
    cmp_bs_1 = vandq_u8(cmp_bs_1, bitmask);
    cmp_qt_0 = vandq_u8(cmp_qt_0, bitmask);
    cmp_qt_1 = vandq_u8(cmp_qt_1, bitmask);

    uint8x16_t sum0 = vpaddq_u8(cmp_bs_0, cmp_bs_1);
    uint8x16_t sum1 = vpaddq_u8(cmp_qt_0, cmp_qt_1);
    sum0 = vpaddq_u8(sum0, sum1);
    sum0 = vpaddq_u8(sum0, sum0);
    return {
      vgetq_lane_u32(vreinterpretq_u32_u8(sum0), 0), // bs_bits
      vgetq_lane_u32(vreinterpretq_u32_u8(sum0), 1) // quote_bits
    };
}

template<>
WARN_UNUSED ALLOW_SAME_PAGE_BUFFER_OVERRUN_QUALIFIER LENIENT_MEM_SANITIZER really_inline
bool parse_string<architecture::arm64>(UNUSED const uint8_t *buf, UNUSED size_t len,
                                ParsedJson &pj, UNUSED const uint32_t depth, UNUSED uint32_t offset) {
  PARSE_STRING(architecture::arm64, buf, len, pj, depth, offset);
}
#endif
#endif