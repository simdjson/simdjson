#ifndef SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H
#define SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#include "simdjson/common_defs.h"
#include "simdjson/constevalutil.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <array>
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__aarch64__) || defined(__ARM_NEON)
  #include <arm_neon.h>
  #define SIMDJSON_KEY_SELECTOR_HAS_NEON 1
#else
  #define SIMDJSON_KEY_SELECTOR_HAS_NEON 0
#endif
#if defined(__SSE2__)
  #include <emmintrin.h>
  #define SIMDJSON_KEY_SELECTOR_HAS_SSE2 1
#else
  #define SIMDJSON_KEY_SELECTOR_HAS_SSE2 0
#endif

#if SIMDJSON_SUPPORTS_CONCEPTS

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

namespace key_selector_detail {

inline constexpr std::size_t MAX_POSITIONS   = 4;
inline constexpr std::size_t MAX_TABLE_SIZE  = 256;
inline constexpr std::uint8_t POS_LAST_CHAR  = 0xFF;
inline constexpr std::uint8_t SENTINEL_KEY   = 0xFF;

// All PHF tables live inside this structural type; a single instance becomes a
// static constexpr member of key_selector<Keys...>, so every field below is a
// compile-time constant at every call site.
template <std::size_t N, std::size_t TableSize, std::size_t MaxKeyLen>
struct phf_data {
    std::array<std::array<std::uint8_t, 256>, MAX_POSITIONS> asso_values{};
    std::array<std::uint8_t, MAX_POSITIONS>                  positions{};
    std::uint8_t                                             num_positions{};
    std::array<std::uint8_t, TableSize>                      slot_to_key{};
    // slot_key_bytes[s] holds the key stored at slot s, zero-padded to MaxKeyLenPadded.
    std::array<std::array<char, ((MaxKeyLen + 15) / 16) * 16>, TableSize> slot_key_bytes{};
    std::array<std::uint8_t, TableSize>                      slot_key_len{};
};

constexpr std::size_t next_pow2(std::size_t n) noexcept {
    std::size_t p = 1;
    while (p < n) p <<= 1;
    return p;
}

// Returns the chosen TableSize (power of two >= N, up to MAX_TABLE_SIZE).
template <std::size_t N>
constexpr std::size_t pick_table_size() noexcept {
    std::size_t t = next_pow2(N);
    if (t < 2) t = 2;
    return t;
}

template <std::size_t N>
constexpr std::size_t char_at(std::string_view key, std::uint8_t pos) noexcept {
    if (pos == POS_LAST_CHAR) {
        return key.empty() ? 256 : static_cast<unsigned char>(key.back());
    }
    return (pos < key.size()) ? static_cast<unsigned char>(key[pos]) : 256;
}

// Try one gperf-style PHF configuration. Returns true if a perfect assignment was found.
template <std::size_t N, std::size_t TableSize>
constexpr bool try_phf(
    const std::array<std::string_view, N>& keys,
    std::array<std::array<std::uint8_t, 256>, MAX_POSITIONS>& asso,
    std::array<std::uint8_t, MAX_POSITIONS>& positions,
    std::uint8_t& num_positions,
    std::array<std::uint8_t, TableSize>& slot_to_key) noexcept
{
    // Helper: reset mapping.
    auto reset = [&]() {
        for (std::size_t i = 0; i < TableSize; ++i) slot_to_key[i] = SENTINEL_KEY;
    };

    // Attempt 1: length-only.
    reset();
    {
        bool ok = true;
        for (std::size_t i = 0; i < N && ok; ++i) {
            std::size_t slot = keys[i].size() % TableSize;
            if (slot_to_key[slot] != SENTINEL_KEY) { ok = false; break; }
            slot_to_key[slot] = static_cast<std::uint8_t>(i);
        }
        if (ok) { num_positions = 0; return true; }
    }

    // Attempt 2: single position (0), vary offset.
    for (std::size_t offset = 0; offset < TableSize; ++offset) {
        reset();
        for (std::size_t c = 0; c < 256; ++c)
            asso[0][c] = static_cast<std::uint8_t>((c + offset) % TableSize);
        positions[0] = 0;
        num_positions = 1;
        bool ok = true;
        for (std::size_t i = 0; i < N && ok; ++i) {
            std::size_t h = keys[i].size();
            std::size_t ch = char_at<N>(keys[i], 0);
            if (ch < 256) h += asso[0][ch];
            std::size_t slot = h % TableSize;
            if (slot_to_key[slot] != SENTINEL_KEY) { ok = false; break; }
            slot_to_key[slot] = static_cast<std::uint8_t>(i);
        }
        if (ok) return true;
    }

    // Attempt 3: positions {0, last_char}.
    for (std::size_t o1 = 0; o1 < TableSize; ++o1) {
        for (std::size_t o2 = 0; o2 < TableSize; ++o2) {
            reset();
            for (std::size_t c = 0; c < 256; ++c) {
                asso[0][c] = static_cast<std::uint8_t>((c + o1) % TableSize);
                asso[1][c] = static_cast<std::uint8_t>((c + o2) % TableSize);
            }
            positions[0] = 0;
            positions[1] = POS_LAST_CHAR;
            num_positions = 2;
            bool ok = true;
            for (std::size_t i = 0; i < N && ok; ++i) {
                std::size_t h = keys[i].size();
                std::size_t c1 = char_at<N>(keys[i], 0);
                if (c1 < 256) h += asso[0][c1];
                std::size_t c2 = char_at<N>(keys[i], POS_LAST_CHAR);
                if (c2 < 256) h += asso[1][c2];
                std::size_t slot = h % TableSize;
                if (slot_to_key[slot] != SENTINEL_KEY) { ok = false; break; }
                slot_to_key[slot] = static_cast<std::uint8_t>(i);
            }
            if (ok) return true;
        }
    }
    return false;
}

template <std::size_t N, std::size_t TableSize, std::size_t MaxKeyLen>
consteval phf_data<N, TableSize, MaxKeyLen>
compute_phf(const std::array<std::string_view, N>& keys) {
    // Validate.
    for (std::size_t i = 0; i < N; ++i) {
        if (keys[i].empty())        throw "empty keys are not allowed in key_selector";
        if (keys[i].size() > MaxKeyLen) throw "key length exceeds MaxKeyLen";
        for (char c : keys[i]) {
            if (c == '\\')          throw "backslash not allowed in key_selector keys";
            if (c == '"')           throw "quote not allowed in key_selector keys";
            if (c == '\0')          throw "null byte not allowed in key_selector keys";
        }
        for (std::size_t j = i + 1; j < N; ++j)
            if (keys[i] == keys[j]) throw "duplicate keys in key_selector";
    }

    phf_data<N, TableSize, MaxKeyLen> out{};
    for (std::size_t s = 0; s < TableSize; ++s) out.slot_to_key[s] = SENTINEL_KEY;

    if (!try_phf<N, TableSize>(keys, out.asso_values, out.positions,
                               out.num_positions, out.slot_to_key))
        throw "key_selector PHF generation failed";

    // Populate slot key bytes (zero-padded) and lengths.
    for (std::size_t s = 0; s < TableSize; ++s) {
        std::uint8_t ki = out.slot_to_key[s];
        if (ki < N) {
            auto k = keys[ki];
            out.slot_key_len[s] = static_cast<std::uint8_t>(k.size());
            for (std::size_t c = 0; c < k.size(); ++c)
                out.slot_key_bytes[s][c] = k[c];
        } else {
            out.slot_key_len[s] = 0; // sentinel: no length can match
        }
    }
    return out;
}

// True if a perfect hash function exists for `keys` at the given table size.
template <std::size_t N, std::size_t TableSize>
consteval bool phf_exists(const std::array<std::string_view, N>& keys) {
    std::array<std::array<std::uint8_t, 256>, MAX_POSITIONS> asso{};
    std::array<std::uint8_t, MAX_POSITIONS>                  positions{};
    std::uint8_t                                             num_positions{};
    std::array<std::uint8_t, TableSize>                      slot_to_key{};
    return try_phf<N, TableSize>(keys, asso, positions, num_positions, slot_to_key);
}

// Smallest power-of-two table size (up to MAX_TABLE_SIZE) for which a perfect
// hash of `keys` exists. The minimal size next_pow2(N) does not always admit one
// (e.g. two keys whose lengths collide modulo 2), so we grow the table until a
// perfect hash is found.
template <std::size_t N>
consteval std::size_t choose_table_size(const std::array<std::string_view, N>& keys) {
    if constexpr (N <=   2) { if (phf_exists<N,   2>(keys)) { return   2; } }
    if constexpr (N <=   4) { if (phf_exists<N,   4>(keys)) { return   4; } }
    if constexpr (N <=   8) { if (phf_exists<N,   8>(keys)) { return   8; } }
    if constexpr (N <=  16) { if (phf_exists<N,  16>(keys)) { return  16; } }
    if constexpr (N <=  32) { if (phf_exists<N,  32>(keys)) { return  32; } }
    if constexpr (N <=  64) { if (phf_exists<N,  64>(keys)) { return  64; } }
    if constexpr (N <= 128) { if (phf_exists<N, 128>(keys)) { return 128; } }
    if (phf_exists<N, 256>(keys)) { return 256; }
    throw "key_selector PHF generation failed";
}

// --- SIMD primitives --------------------------------------------------------

// Scan for the terminating '"' starting at p. Returns its byte offset (= key length).
// Reads at most 16 bytes (if MaxKeyLen <= 15) else up to MaxKeyLen+1 bytes.
// Caller guarantees SIMDJSON_PADDING bytes past the JSON buffer, so the load is safe.
template <std::size_t MaxKeyLen>
simdjson_really_inline std::size_t scan_key_length(const char* p) noexcept {
    static_assert(MaxKeyLen <= 32, "MaxKeyLen must be <= 32 for current SIMD implementations");
#if SIMDJSON_KEY_SELECTOR_HAS_NEON
    uint8x16_t v0 = vld1q_u8(reinterpret_cast<const uint8_t*>(p));
    uint8x16_t cmp0 = vceqq_u8(v0, vdupq_n_u8('"'));
    uint64_t m0 = vget_lane_u64(
        vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp0), 4)), 0);
    if constexpr (MaxKeyLen < 16) {
        // Only the first 16 bytes are relevant.
        if (simdjson_likely(m0 != 0)) return std::size_t(__builtin_ctzll(m0)) >> 2;
        return MaxKeyLen + 1;
    } else {
        uint8x16_t v1 = vld1q_u8(reinterpret_cast<const uint8_t*>(p) + 16);
        uint8x16_t cmp1 = vceqq_u8(v1, vdupq_n_u8('"'));
        uint64_t m1 = vget_lane_u64(
            vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp1), 4)), 0);
        // Combine into a single 128-bit-ish mask. If m0 != 0, first-byte lives there.
        if (simdjson_likely(m0 != 0)) return std::size_t(__builtin_ctzll(m0)) >> 2;
        if (m1 != 0) return 16 + (std::size_t(__builtin_ctzll(m1)) >> 2);
        return MaxKeyLen + 1;
    }
#elif SIMDJSON_KEY_SELECTOR_HAS_SSE2
    __m128i v0   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
    __m128i cmp0 = _mm_cmpeq_epi8(v0, _mm_set1_epi8('"'));
    unsigned m0  = static_cast<unsigned>(_mm_movemask_epi8(cmp0));
    if constexpr (MaxKeyLen < 16) {
        if (simdjson_likely(m0 != 0)) return std::size_t(__builtin_ctz(m0));
        return MaxKeyLen + 1;
    } else {
        __m128i v1   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 16));
        __m128i cmp1 = _mm_cmpeq_epi8(v1, _mm_set1_epi8('"'));
        unsigned m1  = static_cast<unsigned>(_mm_movemask_epi8(cmp1));
        if (simdjson_likely(m0 != 0)) return std::size_t(__builtin_ctz(m0));
        if (m1 != 0) return 16 + std::size_t(__builtin_ctz(m1));
        return MaxKeyLen + 1;
    }
#else
    for (std::size_t i = 0; i <= MaxKeyLen; ++i)
        if (p[i] == '"') return i;
    return MaxKeyLen + 1;
#endif
}

// Byte-equal of p[0..len) against stored[0..len). stored is zero-padded past `len`.
// Input is read over 16 or 32 bytes (padded JSON buffer guaranteed).
template <std::size_t MaxKeyLen>
simdjson_really_inline bool compare_key_bytes(
    const char* p, const char* stored, std::size_t len) noexcept
{
    alignas(16) static constexpr uint8_t idx16[16] =
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    if constexpr (MaxKeyLen <= 16) {
#if SIMDJSON_KEY_SELECTOR_HAS_NEON
        uint8x16_t vp = vld1q_u8(reinterpret_cast<const uint8_t*>(p));
        uint8x16_t vs = vld1q_u8(reinterpret_cast<const uint8_t*>(stored));
        uint8x16_t mask = vcltq_u8(vld1q_u8(idx16), vdupq_n_u8(static_cast<uint8_t>(len)));
        uint8x16_t diff = veorq_u8(vandq_u8(vp, mask), vs);
        return vmaxvq_u8(diff) == 0;
#elif SIMDJSON_KEY_SELECTOR_HAS_SSE2
        __m128i vp = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i vs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(stored));
        __m128i idx = _mm_load_si128(reinterpret_cast<const __m128i*>(idx16));
        __m128i mask = _mm_cmplt_epi8(idx, _mm_set1_epi8(static_cast<char>(len)));
        __m128i eq = _mm_cmpeq_epi8(_mm_and_si128(vp, mask), vs);
        return _mm_movemask_epi8(eq) == 0xFFFF;
#else
        for (std::size_t i = 0; i < len; ++i)
            if (p[i] != stored[i]) return false;
        return true;
#endif
    } else if constexpr (MaxKeyLen <= 32) {
        // Two 16-byte lanes. JSON buffer is padded so the second load is safe.
        alignas(16) static constexpr uint8_t idx32_hi[16] =
            {16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31};
#if SIMDJSON_KEY_SELECTOR_HAS_NEON
        uint8x16_t vp_lo = vld1q_u8(reinterpret_cast<const uint8_t*>(p));
        uint8x16_t vp_hi = vld1q_u8(reinterpret_cast<const uint8_t*>(p) + 16);
        uint8x16_t vs_lo = vld1q_u8(reinterpret_cast<const uint8_t*>(stored));
        uint8x16_t vs_hi = vld1q_u8(reinterpret_cast<const uint8_t*>(stored) + 16);
        uint8x16_t lenv = vdupq_n_u8(static_cast<uint8_t>(len));
        uint8x16_t m_lo = vcltq_u8(vld1q_u8(idx16),    lenv);
        uint8x16_t m_hi = vcltq_u8(vld1q_u8(idx32_hi), lenv);
        uint8x16_t d_lo = veorq_u8(vandq_u8(vp_lo, m_lo), vs_lo);
        uint8x16_t d_hi = veorq_u8(vandq_u8(vp_hi, m_hi), vs_hi);
        return vmaxvq_u8(vorrq_u8(d_lo, d_hi)) == 0;
#elif SIMDJSON_KEY_SELECTOR_HAS_SSE2
        __m128i vp_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i vp_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + 16));
        __m128i vs_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(stored));
        __m128i vs_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(stored + 16));
        __m128i lenv = _mm_set1_epi8(static_cast<char>(len));
        __m128i m_lo = _mm_cmplt_epi8(_mm_load_si128(reinterpret_cast<const __m128i*>(idx16)),    lenv);
        __m128i m_hi = _mm_cmplt_epi8(_mm_load_si128(reinterpret_cast<const __m128i*>(idx32_hi)), lenv);
        __m128i eq_lo = _mm_cmpeq_epi8(_mm_and_si128(vp_lo, m_lo), vs_lo);
        __m128i eq_hi = _mm_cmpeq_epi8(_mm_and_si128(vp_hi, m_hi), vs_hi);
        return (_mm_movemask_epi8(eq_lo) & _mm_movemask_epi8(eq_hi)) == 0xFFFF;
#else
        for (std::size_t i = 0; i < len; ++i)
            if (p[i] != stored[i]) return false;
        return true;
#endif
    } else {
        // MaxKeyLen > 32: byte loop.
        for (std::size_t i = 0; i < len; ++i)
            if (p[i] != stored[i]) return false;
        return true;
    }
}

template <std::size_t N>
constexpr std::size_t compute_max_key_len(const std::array<std::string_view, N>& keys) noexcept {
    std::size_t m = 0;
    for (std::size_t i = 0; i < N; ++i) if (keys[i].size() > m) m = keys[i].size();
    return m;
}

} // namespace key_selector_detail

/**
 * Stateless, compile-time key selector.
 *
 * Usage:
 *   using sel_t = key_selector<"id", "text", "user">;
 *   std::size_t i = sel_t::match_raw(raw_key); // returns sel_t::size() on miss
 *
 * All PHF tables are static constexpr — the compiler sees them as compile-time
 * constants at every call site and fully unrolls compute_hash / compare.
 *
 * Limitations:
 *   - Each key must be at most 32 characters long (and no longer than
 *     SIMDJSON_PADDING). Longer keys trigger a compile-time error.
 *   - The number of keys should be moderate. The hard limit is 100 keys, but the
 *     compile-time perfect-hash construction may fail (compile-time error) for
 *     large or awkward key sets, and compilation time grows with the number of
 *     keys, so prefer a handful of keys per selector.
 *   - Keys must be distinct, non-empty, and free of backslash, double-quote and
 *     null bytes (matching is done against the raw, unescaped JSON key bytes).
 */
template <constevalutil::fixed_string... Keys>
struct key_selector {
    static constexpr std::size_t N = sizeof...(Keys);
    static_assert(N > 0,   "key_selector requires at least one key");
    static_assert(N <= 100,"key_selector supports at most 100 keys");

    static constexpr std::array<std::string_view, N> keys{ Keys.view()... };
    static constexpr std::size_t table_size  = key_selector_detail::choose_table_size<N>(keys);
    static constexpr std::size_t max_key_len = key_selector_detail::compute_max_key_len<N>(keys);
    static_assert(max_key_len <= SIMDJSON_PADDING,
                  "key longer than SIMDJSON_PADDING is not supported");

    static constexpr auto phf =
        key_selector_detail::compute_phf<N, table_size, max_key_len>(keys);

    static constexpr std::size_t size() noexcept { return N; }

    /**
     * Look up a JSON key. rjs must point just after an opening quote in a padded
     * simdjson buffer. Returns the selector index in [0, N) on match, or N on miss.
     */
    static simdjson_really_inline std::size_t match_raw(raw_json_string rjs) noexcept {
        const char* p = rjs.raw();
        std::size_t len = key_selector_detail::scan_key_length<max_key_len>(p);
        if (len == 0 || len > max_key_len) return N;
        // Compute hash. positions / num_positions / asso_values are compile-time
        // constants, so this fully unrolls.
        std::size_t h = len;
        for (std::uint8_t i = 0; i < phf.num_positions; ++i) {
            std::uint8_t pos = phf.positions[i];
            std::size_t idx = (pos == key_selector_detail::POS_LAST_CHAR)
                              ? (len - std::size_t{1})
                              : static_cast<std::size_t>(pos);
            h += phf.asso_values[i][p[idx]];
        }
        std::size_t slot = h & (table_size - 1);
        std::uint8_t ki = phf.slot_to_key[slot];
        if (ki >= N) return N;
        if (phf.slot_key_len[slot] != len) return N;
        if (!key_selector_detail::compare_key_bytes<max_key_len>(
                p, phf.slot_key_bytes[slot].data(), len)) return N;
        return ki;
    }

    /** Return the key text at selector index i (i in [0, N)). */
    static constexpr std::string_view key_at(std::size_t i) noexcept {
        return keys[i];
    }
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_CONCEPTS

#endif // SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H
