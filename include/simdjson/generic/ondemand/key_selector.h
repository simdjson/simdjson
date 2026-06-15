#ifndef SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H
#define SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H

#ifndef SIMDJSON_CONDITIONAL_INCLUDE
#include "simdjson/base.h"
#include "simdjson/common_defs.h"
#include "simdjson/constevalutil.h"
#include "simdjson/generic/ondemand/raw_json_string.h"
#endif // SIMDJSON_CONDITIONAL_INCLUDE

#include <array>
#include <string>      // std::string (key_selector::describe)
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <cstring>     // std::memcpy (portable unaligned window load)
#include <utility>     // std::index_sequence (window candidate dispatch)
#include <type_traits> // std::integral_constant (window candidate dispatch)

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
#if defined(__loongarch_sx)
  #include <lsxintrin.h>
  #define SIMDJSON_KEY_SELECTOR_HAS_LSX 1
#else
  #define SIMDJSON_KEY_SELECTOR_HAS_LSX 0
#endif

#if SIMDJSON_SUPPORTS_CONCEPTS

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {

namespace key_selector_detail {

// ============================================================================
// Compile-time perfect-hash generator.
// It scales to ~100 keys at compile time by determining association values one (position, character)
// symbol at a time (gperf-style) instead of an exhaustive offset search, and
// falls back to a Hash-and-Displace construction for large/awkward key sets.
//
// Only flat tables survive to runtime; the lookup is a few additions plus a
// single SIMD key comparison (see match_raw below).
// ============================================================================

// Maximum number of character positions the gperf hash may combine.
static constexpr std::size_t MAX_POSITIONS = 16;
// Sentinel "position" meaning "the last character of the key".
static constexpr std::size_t LAST_CHAR = std::size_t(-1);
// Runtime-encoded sentinels (stored in uint8 tables).
static constexpr std::uint8_t POS_LAST_CHAR = 0xFF; // positions_[i] == last char
static constexpr std::uint8_t HD_MODE       = 0xFF; // num_positions == H&D mode
// Flags stored in positions[2] in H&D mode to select the key-hash variant.
static constexpr std::size_t HD_HASH_2BYTE_FLAG = 2;
static constexpr std::size_t HD_HASH_4BYTE_FLAG = 4;

constexpr std::size_t next_power_of_2(std::size_t n) noexcept {
    if (n == 0) { return 1; }
    std::size_t p = 1;
    while (p < n) { p <<= 1; }
    return p;
}

// Character at a given position (LAST_CHAR means last character), or 256 if out
// of bounds.
constexpr std::size_t char_at(std::string_view key, std::size_t pos) noexcept {
    if (pos == LAST_CHAR) {
        if (key.empty()) { return 256; }
        return static_cast<unsigned char>(key[key.size() - 1]);
    }
    if (pos >= key.size()) { return 256; }
    return static_cast<unsigned char>(key[pos]);
}

// Count key pairs that a set of positions fails to distinguish. Keys whose
// lengths differ modulo the table size are separated by the length term in the
// hash, so they need no position coverage.
template <std::size_t N>
consteval std::size_t count_undistinguished_pairs(
    const std::array<std::string_view, N>& keys,
    const std::size_t* positions,
    std::size_t num_positions,
    std::size_t modulus) {
    std::size_t count = 0;
    for (std::size_t i = 0; i < N; ++i) {
        for (std::size_t j = i + 1; j < N; ++j) {
            if (keys[i].size() % modulus != keys[j].size() % modulus) { continue; }
            bool distinguished = false;
            for (std::size_t p = 0; p < num_positions; ++p) {
                if (char_at(keys[i], positions[p]) != char_at(keys[j], positions[p])) {
                    distinguished = true;
                    break;
                }
            }
            if (!distinguished) { ++count; }
        }
    }
    return count;
}

template <std::size_t N>
consteval bool positions_distinguish(
    const std::array<std::string_view, N>& keys,
    const std::size_t* positions,
    std::size_t num_positions,
    std::size_t modulus) {
    return count_undistinguished_pairs<N>(keys, positions, num_positions, modulus) == 0;
}

// Number of distinct (length % modulus, char_at(key, pos)) pairs at a position.
template <std::size_t N>
consteval std::size_t discriminating_power(
    const std::array<std::string_view, N>& keys,
    std::size_t pos,
    std::size_t modulus) {
    struct pair { std::size_t len_mod; std::size_t ch; };
    std::array<pair, N> pairs{};
    for (std::size_t i = 0; i < N; ++i) {
        pairs[i] = {keys[i].size() % modulus, char_at(keys[i], pos)};
    }
    std::size_t count = 0;
    for (std::size_t i = 0; i < N; ++i) {
        bool dup = false;
        for (std::size_t j = 0; j < i; ++j) {
            if (pairs[i].len_mod == pairs[j].len_mod && pairs[i].ch == pairs[j].ch) {
                dup = true;
                break;
            }
        }
        if (!dup) { ++count; }
    }
    return count;
}

template <std::size_t N>
consteval std::size_t max_key_length(const std::array<std::string_view, N>& keys) {
    std::size_t m = 0;
    for (std::size_t i = 0; i < N; ++i) {
        if (keys[i].size() > m) { m = keys[i].size(); }
    }
    return m;
}

// Bounded backtracking DFS for a minimal set of distinguishing positions.
template <std::size_t N>
consteval bool backtracking_search(
    const std::array<std::string_view, N>& keys,
    const std::size_t* candidates,
    std::size_t num_candidates,
    std::size_t* positions,
    std::size_t& num_positions_out,
    std::size_t& budget,
    std::size_t modulus) {
    constexpr std::size_t MAX_DEPTH = 8;
    std::size_t breadth = num_candidates < 20 ? num_candidates : 20;

    struct frame { std::size_t depth; std::size_t next_ci; std::size_t parent_count; };
    std::array<frame, MAX_DEPTH + 1> stack{};
    std::size_t sp = 0;

    std::size_t initial_count = count_undistinguished_pairs<N>(keys, positions, 0, modulus);
    if (budget > 0) { --budget; }
    if (initial_count == 0) { num_positions_out = 0; return true; }

    stack[0] = {0, 0, initial_count};

    while (budget > 0) {
        if (sp > MAX_DEPTH) {
            if (sp == 0) { break; }
            --sp;
            ++stack[sp].next_ci;
            continue;
        }
        auto& f = stack[sp];
        if (f.next_ci >= breadth) {
            if (sp == 0) { break; }
            --sp;
            ++stack[sp].next_ci;
            continue;
        }
        positions[sp] = candidates[f.next_ci];
        --budget;
        std::size_t new_count = count_undistinguished_pairs<N>(keys, positions, sp + 1, modulus);
        if (new_count == 0) { num_positions_out = sp + 1; return true; }
        if (new_count < f.parent_count && sp + 1 < MAX_DEPTH) {
            stack[sp + 1] = {sp + 1, f.next_ci + 1, new_count};
            ++sp;
        } else {
            ++f.next_ci;
        }
    }
    return false;
}

// Phase 1: select character positions that distinguish all colliding pairs.
template <std::size_t N>
consteval std::size_t select_positions(
    const std::array<std::string_view, N>& keys,
    std::array<std::size_t, MAX_POSITIONS>& positions,
    std::size_t modulus) {
    if (positions_distinguish<N>(keys, positions.data(), 0, modulus)) { return 0; }

    std::size_t max_len = max_key_length(keys);
    constexpr std::size_t MAX_CANDIDATES = 256;
    std::array<std::size_t, MAX_CANDIDATES> candidates{};
    std::array<std::size_t, MAX_CANDIDATES> powers{};
    std::size_t num_candidates = 0;
    for (std::size_t p = 0; p < max_len && num_candidates < MAX_CANDIDATES - 1; ++p) {
        candidates[num_candidates] = p;
        powers[num_candidates] = discriminating_power(keys, p, modulus);
        ++num_candidates;
    }
    if (num_candidates < MAX_CANDIDATES) {
        candidates[num_candidates] = LAST_CHAR;
        powers[num_candidates] = discriminating_power(keys, LAST_CHAR, modulus);
        ++num_candidates;
    }
    for (std::size_t i = 0; i < num_candidates; ++i) {
        for (std::size_t j = i + 1; j < num_candidates; ++j) {
            if (powers[j] > powers[i]) {
                auto tc = candidates[i]; candidates[i] = candidates[j]; candidates[j] = tc;
                auto tp = powers[i]; powers[i] = powers[j]; powers[j] = tp;
            }
        }
    }

    positions[0] = candidates[0];
    if (positions_distinguish<N>(keys, positions.data(), 1, modulus)) { return 1; }

    positions[0] = 0;
    positions[1] = LAST_CHAR;
    if (positions_distinguish<N>(keys, positions.data(), 2, modulus)) { return 2; }

    {
        std::size_t budget = 5000;
        std::size_t num_found = 0;
        if (backtracking_search<N>(keys, candidates.data(), num_candidates,
                                   positions.data(), num_found, budget, modulus)) {
            return num_found;
        }
    }

    std::size_t num_pos = 0;
    for (std::size_t ci = 0; ci < num_candidates && num_pos < MAX_POSITIONS; ++ci) {
        bool already = false;
        for (std::size_t p = 0; p < num_pos; ++p) {
            if (positions[p] == candidates[ci]) { already = true; break; }
        }
        if (already) { continue; }
        positions[num_pos] = candidates[ci];
        ++num_pos;
        if (positions_distinguish<N>(keys, positions.data(), num_pos, modulus)) { return num_pos; }
    }

    throw "Failed to find distinguishing positions for perfect hash";
}

// Result of PHF computation. A max-sized slot_to_key array lets the same struct
// type carry any chosen table size.
template <std::size_t N>
struct phf_result {
    // Allow up to 8x the minimum table size. Sparser tables solve faster.
    static constexpr std::size_t MAX_TABLE_SIZE = next_power_of_2(N) * 8;
    std::size_t table_size{};
    std::array<std::array<std::size_t, 256>, MAX_POSITIONS> asso_values{};
    std::size_t num_positions{};
    std::array<std::size_t, MAX_POSITIONS> positions{};
    std::array<std::size_t, MAX_TABLE_SIZE> slot_to_key{};
};

// Partition-based asso_values search (gperf-style). Determines asso_values one
// (position, character) symbol at a time; never revisits a value. Equivalence
// classes (keys sharing the same undetermined symbols) keep the search cheap.
template <std::size_t N, std::size_t M>
consteval bool try_generate_gperf(
    const std::array<std::string_view, N>& keys,
    std::array<std::array<std::size_t, 256>, MAX_POSITIONS>& asso_values,
    std::size_t& num_positions,
    std::array<std::size_t, MAX_POSITIONS>& positions,
    std::array<std::size_t, M>& slot_to_key) {
    num_positions = select_positions<N>(keys, positions, M);

    for (std::size_t p = 0; p < MAX_POSITIONS; ++p) {
        for (std::size_t c = 0; c < 256; ++c) { asso_values[p][c] = 0; }
    }

    if (num_positions == 0) {
        for (std::size_t i = 0; i < M; ++i) { slot_to_key[i] = N; }
        for (std::size_t i = 0; i < N; ++i) {
            std::size_t slot = keys[i].size() % M;
            if (slot_to_key[slot] != N) { return false; }
            slot_to_key[slot] = i;
        }
        return true;
    }

    std::array<std::array<std::size_t, MAX_POSITIONS>, N> kchars{};
    for (std::size_t k = 0; k < N; ++k) {
        for (std::size_t p = 0; p < num_positions; ++p) {
            kchars[k][p] = char_at(keys[k], positions[p]);
        }
    }

    struct sym_t { std::size_t pos; std::size_t ch; std::size_t freq; };
    constexpr std::size_t MAX_SYMS = MAX_POSITIONS * 256;
    std::array<sym_t, MAX_SYMS> syms{};
    std::size_t nsyms = 0;
    for (std::size_t p = 0; p < num_positions; ++p) {
        std::array<std::size_t, 256> freq{};
        for (std::size_t k = 0; k < N; ++k) {
            std::size_t c = kchars[k][p];
            if (c < 256) { freq[c]++; }
        }
        for (std::size_t c = 0; c < 256; ++c) {
            if (freq[c] > 0) { syms[nsyms++] = {p, c, freq[c]}; }
        }
    }
    for (std::size_t i = 0; i < nsyms; ++i) {
        for (std::size_t j = i + 1; j < nsyms; ++j) {
            if (syms[j].freq > syms[i].freq) {
                auto tmp = syms[i]; syms[i] = syms[j]; syms[j] = tmp;
            }
        }
    }

    std::array<std::size_t, N> phash{};
    for (std::size_t k = 0; k < N; ++k) { phash[k] = keys[k].size(); }

    std::array<std::array<uint64_t, 256>, MAX_POSITIONS> salt{};
    {
        uint64_t s = 0x9e3779b97f4a7c15ULL;
        for (std::size_t p = 0; p < num_positions; ++p) {
            for (std::size_t c = 0; c < 256; ++c) {
                s = s * 6364136223846793005ULL + 1442695040888963407ULL;
                salt[p][c] = s;
            }
        }
    }
    std::array<uint64_t, N> sig{};
    for (std::size_t k = 0; k < N; ++k) {
        uint64_t s = 0;
        for (std::size_t p = 0; p < num_positions; ++p) {
            std::size_t c = kchars[k][p];
            if (c < 256) { s ^= salt[p][c]; }
        }
        sig[k] = s;
    }
    std::array<std::size_t, N> order{};
    for (std::size_t k = 0; k < N; ++k) { order[k] = k; }

    std::array<std::size_t, M> slot_gen{};
    std::size_t gen = 0;

    std::size_t search_limit = next_power_of_2(M);
    if (search_limit < 32) { search_limit = 32; }

    for (std::size_t si = 0; si < nsyms; ++si) {
        std::size_t sp = syms[si].pos;
        std::size_t sc = syms[si].ch;

        uint64_t sp_salt = salt[sp][sc];
        for (std::size_t k = 0; k < N; ++k) {
            if (kchars[k][sp] == sc) { sig[k] ^= sp_salt; }
        }

        for (std::size_t i = 1; i < N; ++i) {
            std::size_t x = order[i];
            uint64_t xs = sig[x];
            std::size_t j = i;
            while (j > 0 && sig[order[j - 1]] > xs) {
                order[j] = order[j - 1];
                --j;
            }
            order[j] = x;
        }

        bool found = false;
        for (std::size_t v = 0; v < search_limit && !found; ++v) {
            bool collision = false;
            std::size_t ci = 0;
            while (ci < N && !collision) {
                uint64_t class_sig = sig[order[ci]];
                std::size_t cj = ci;
                while (cj < N && sig[order[cj]] == class_sig) { ++cj; }
                if (cj - ci > 1) {
                    ++gen;
                    for (std::size_t x = ci; x < cj; ++x) {
                        std::size_t k = order[x];
                        std::size_t h = phash[k];
                        if (kchars[k][sp] == sc) { h += v; }
                        h %= M;
                        if (slot_gen[h] == gen) { collision = true; break; }
                        slot_gen[h] = gen;
                    }
                }
                ci = cj;
            }
            if (!collision) {
                asso_values[sp][sc] = v;
                for (std::size_t k = 0; k < N; ++k) {
                    if (kchars[k][sp] == sc) { phash[k] += v; }
                }
                found = true;
            }
        }
        if (!found) { return false; }
    }

    for (std::size_t i = 0; i < M; ++i) { slot_to_key[i] = N; }
    for (std::size_t i = 0; i < N; ++i) {
        std::size_t slot = phash[i] % M;
        if (slot_to_key[slot] != N) { return false; }
        slot_to_key[slot] = i;
    }
    std::size_t filled = 0;
    for (std::size_t i = 0; i < M; ++i) {
        if (slot_to_key[i] != N) { ++filled; }
    }
    return filled == N;
}

template <std::size_t N, std::size_t M>
consteval bool try_compute_phf(const std::array<std::string_view, N>& keys, phf_result<N>& result) {
    static_assert(M <= phf_result<N>::MAX_TABLE_SIZE, "Table size M exceeds maximum");
    std::array<std::array<std::size_t, 256>, MAX_POSITIONS> asso{};
    std::size_t npos{};
    std::array<std::size_t, MAX_POSITIONS> pos{};
    std::array<std::size_t, M> s2k{};
    if (try_generate_gperf<N, M>(keys, asso, npos, pos, s2k)) {
        result.table_size = M;
        result.asso_values = asso;
        result.num_positions = npos;
        result.positions = pos;
        for (std::size_t i = 0; i < M; ++i) { result.slot_to_key[i] = s2k[i]; }
        for (std::size_t i = M; i < phf_result<N>::MAX_TABLE_SIZE; ++i) { result.slot_to_key[i] = N; }
        return true;
    }
    return false;
}

template <std::size_t N, std::size_t M, std::size_t MaxM>
consteval bool try_gperf_po2(const std::array<std::string_view, N>& keys, phf_result<N>& result) {
    if (try_compute_phf<N, M>(keys, result)) { return true; }
    constexpr std::size_t NextM = M * 2;
    if constexpr (NextM <= MaxM) { return try_gperf_po2<N, NextM, MaxM>(keys, result); }
    return false;
}

// --- Hash-and-Displace fallback --------------------------------------------

constexpr std::size_t hd_bucket_hash(std::string_view key) noexcept {
    std::size_t c0 = key.empty() ? 0 : static_cast<unsigned char>(key[0]);
    std::size_t c1 = key.empty() ? 0 : static_cast<unsigned char>(key[key.size() - 1]);
    return (c0 + c1 * 3 + key.size() * 17) & 0xFF;
}
constexpr std::size_t hd_safe_char(const char* p, std::size_t len, std::size_t idx) noexcept {
    std::size_t has = static_cast<std::size_t>(idx < len);
    std::size_t si = idx & (std::size_t{0} - has);
    return static_cast<unsigned char>(p[si]) & (std::size_t{0} - has);
}
constexpr std::size_t hd_key_hash_2(std::string_view key) noexcept {
    std::size_t kc = key.size();
    kc = kc * 31 + static_cast<unsigned char>(key[0]);
    kc = kc * 31 + hd_safe_char(key.data(), key.size(), 1);
    return kc;
}
constexpr std::size_t hd_key_hash_4(std::string_view key) noexcept {
    std::size_t kc = key.size();
    kc = kc * 31 + static_cast<unsigned char>(key[0]);
    kc = kc * 31 + hd_safe_char(key.data(), key.size(), 1);
    kc = kc * 31 + hd_safe_char(key.data(), key.size(), 2);
    kc = kc * 31 + hd_safe_char(key.data(), key.size(), 3);
    return kc;
}

template <std::size_t N, std::size_t M>
consteval bool try_hash_and_displace(
    const std::array<std::string_view, N>& keys,
    std::array<std::array<std::size_t, 256>, MAX_POSITIONS>& asso_values,
    std::size_t& num_positions,
    std::array<std::size_t, MAX_POSITIONS>& positions,
    std::array<std::size_t, M>& slot_to_key) {
    num_positions = select_positions<N>(keys, positions, M);

    if (num_positions == 0) {
        for (std::size_t i = 0; i < 256; ++i) { asso_values[0][i] = 0; }
        for (std::size_t i = 0; i < M; ++i) { slot_to_key[i] = N; }
        for (std::size_t i = 0; i < N; ++i) {
            std::size_t slot = keys[i].size() % M;
            if (slot_to_key[slot] != N) { return false; }
            slot_to_key[slot] = i;
        }
        return true;
    }

    for (std::size_t i = 0; i < 256; ++i) { asso_values[0][i] = 0; }
    num_positions = HD_MODE; // sentinel for H&D mode
    positions[0] = 0;
    positions[1] = LAST_CHAR;

    std::array<std::size_t, N> key_bucket{};
    for (std::size_t i = 0; i < N; ++i) { key_bucket[i] = hd_bucket_hash(keys[i]); }

    struct bucket_info { std::size_t ch; std::size_t count; };
    std::array<bucket_info, N> buckets{};
    std::size_t num_buckets = 0;
    for (std::size_t i = 0; i < N; ++i) {
        std::size_t bk = key_bucket[i];
        bool found = false;
        for (std::size_t b = 0; b < num_buckets; ++b) {
            if (buckets[b].ch == bk) { ++buckets[b].count; found = true; break; }
        }
        if (!found) { buckets[num_buckets++] = {bk, 1}; }
    }
    for (std::size_t i = 0; i < num_buckets; ++i) {
        for (std::size_t j = i + 1; j < num_buckets; ++j) {
            if (buckets[j].count > buckets[i].count) {
                auto tmp = buckets[i]; buckets[i] = buckets[j]; buckets[j] = tmp;
            }
        }
    }

    auto try_placement = [&](auto key_hash_fn) -> bool {
        for (std::size_t i = 0; i < M; ++i) { slot_to_key[i] = N; }
        for (std::size_t i = 0; i < 256; ++i) { asso_values[0][i] = 0; }
        for (std::size_t b = 0; b < num_buckets; ++b) {
            std::size_t ch = buckets[b].ch;
            std::array<std::size_t, N> bucket_keys{};
            std::size_t bk_count = 0;
            for (std::size_t i = 0; i < N; ++i) {
                if (key_bucket[i] == ch) { bucket_keys[bk_count++] = i; }
            }
            bool placed = false;
            std::size_t max_d = M < 255 ? M : 255;
            for (std::size_t d = 0; d < max_d; ++d) {
                bool ok = true;
                std::array<std::size_t, N> bucket_slots{};
                for (std::size_t k = 0; k < bk_count; ++k) {
                    std::size_t slot = (d + key_hash_fn(keys[bucket_keys[k]])) % M;
                    if (slot_to_key[slot] != N) { ok = false; break; }
                    for (std::size_t k2 = 0; k2 < k; ++k2) {
                        if (bucket_slots[k2] == slot) { ok = false; break; }
                    }
                    if (!ok) { break; }
                    bucket_slots[k] = slot;
                }
                if (ok) {
                    asso_values[0][ch] = d;
                    for (std::size_t k = 0; k < bk_count; ++k) {
                        slot_to_key[bucket_slots[k]] = bucket_keys[k];
                    }
                    placed = true;
                    break;
                }
            }
            if (!placed) { return false; }
        }
        std::size_t filled = 0;
        for (std::size_t i = 0; i < M; ++i) {
            if (slot_to_key[i] != N) { ++filled; }
        }
        return filled == N;
    };

    if (try_placement([](std::string_view k) { return hd_key_hash_2(k); })) {
        positions[2] = HD_HASH_2BYTE_FLAG;
        return true;
    }
    if (try_placement([](std::string_view k) { return hd_key_hash_4(k); })) {
        positions[2] = HD_HASH_4BYTE_FLAG;
        return true;
    }
    return false;
}

template <std::size_t N, std::size_t M>
consteval bool try_compute_phf_hd(const std::array<std::string_view, N>& keys, phf_result<N>& result) {
    static_assert(M <= phf_result<N>::MAX_TABLE_SIZE, "Table size M exceeds maximum");
    std::array<std::array<std::size_t, 256>, MAX_POSITIONS> asso{};
    std::size_t npos{};
    std::array<std::size_t, MAX_POSITIONS> pos{};
    std::array<std::size_t, M> s2k{};
    if (try_hash_and_displace<N, M>(keys, asso, npos, pos, s2k)) {
        result.table_size = M;
        result.asso_values = asso;
        result.num_positions = npos;
        result.positions = pos;
        for (std::size_t i = 0; i < M; ++i) { result.slot_to_key[i] = s2k[i]; }
        for (std::size_t i = M; i < phf_result<N>::MAX_TABLE_SIZE; ++i) { result.slot_to_key[i] = N; }
        return true;
    }
    return false;
}

template <std::size_t N, std::size_t M>
consteval phf_result<N> compute_phf_hd_po2(const std::array<std::string_view, N>& keys) {
    static_assert(M <= phf_result<N>::MAX_TABLE_SIZE, "Table size M exceeds maximum");
    phf_result<N> result{};
    if (try_compute_phf_hd<N, M>(keys, result)) { return result; }
    constexpr std::size_t NextM = M * 2;
    if constexpr (NextM <= phf_result<N>::MAX_TABLE_SIZE) {
        return compute_phf_hd_po2<N, NextM>(keys);
    } else {
        throw "Hash-and-Displace: failed to find valid table size";
    }
}

// Compute a perfect hash for `keys`: try gperf at power-of-two sizes (capped so
// the runtime tables stay within uint8 indices), then fall back to H&D.
template <std::size_t N>
consteval phf_result<N> compute_phf(const std::array<std::string_view, N>& keys) {
    constexpr std::size_t StartM = next_power_of_2(N);
    constexpr std::size_t GPERF_MAX_TABLE =
        phf_result<N>::MAX_TABLE_SIZE < 256 ? phf_result<N>::MAX_TABLE_SIZE : 256;
    if constexpr (StartM <= GPERF_MAX_TABLE) {
        phf_result<N> result{};
        if (try_gperf_po2<N, StartM, GPERF_MAX_TABLE>(keys, result)) { return result; }
    }
    return compute_phf_hd_po2<N, StartM>(keys);
}

// ============================================================================
// Runtime tables (flat, uint8) derived from a phf_result.
// ============================================================================

template <std::size_t N, std::size_t TableSize, std::size_t MaxKeyLen>
struct phf_data {
    std::array<std::array<std::uint8_t, 256>, MAX_POSITIONS> asso_values{};
    std::array<std::uint8_t, MAX_POSITIONS>                  positions{};
    std::uint8_t                                             num_positions{};
    std::uint8_t                                             hd_hash_variant{}; // 2 or 4 (H&D only)
    std::array<std::uint8_t, TableSize>                      slot_to_key{};
    // slot_key_bytes[s] holds the key stored at slot s, zero-padded to a 16-byte
    // multiple so the SIMD comparison can read a whole register.
    std::array<std::array<char, ((MaxKeyLen + 15) / 16) * 16>, TableSize> slot_key_bytes{};
    std::array<std::uint8_t, TableSize>                      slot_key_len{};
};

template <std::size_t N>
constexpr std::size_t compute_max_key_len(const std::array<std::string_view, N>& keys) noexcept {
    std::size_t m = 0;
    for (std::size_t i = 0; i < N; ++i) { if (keys[i].size() > m) { m = keys[i].size(); } }
    return m;
}

// Validate keys and build the runtime tables from the computed perfect hash.
template <std::size_t N, std::size_t TableSize, std::size_t MaxKeyLen>
consteval phf_data<N, TableSize, MaxKeyLen>
build_phf_data(const std::array<std::string_view, N>& keys, const phf_result<N>& result) {
    for (std::size_t i = 0; i < N; ++i) {
        if (keys[i].empty())            { throw "empty keys are not allowed in key_selector"; }
        if (keys[i].size() > MaxKeyLen) { throw "key length exceeds MaxKeyLen"; }
        for (char c : keys[i]) {
            if (c == '\\') { throw "backslash not allowed in key_selector keys"; }
            if (c == '"')  { throw "quote not allowed in key_selector keys"; }
            if (c == '\0') { throw "null byte not allowed in key_selector keys"; }
        }
        for (std::size_t j = i + 1; j < N; ++j) {
            if (keys[i] == keys[j]) { throw "duplicate keys in key_selector"; }
        }
    }

    phf_data<N, TableSize, MaxKeyLen> out{};

    if (result.num_positions == HD_MODE) {
        // H&D mode: single displacement table in asso_values[0].
        for (std::size_t c = 0; c < 256; ++c) {
            out.asso_values[0][c] = static_cast<std::uint8_t>(result.asso_values[0][c]);
        }
        out.num_positions   = static_cast<std::uint8_t>(HD_MODE);
        out.hd_hash_variant = static_cast<std::uint8_t>(result.positions[2]);
    } else {
        for (std::size_t pi = 0; pi < result.num_positions; ++pi) {
            for (std::size_t c = 0; c < 256; ++c) {
                out.asso_values[pi][c] = static_cast<std::uint8_t>(result.asso_values[pi][c] % TableSize);
            }
        }
        out.num_positions = static_cast<std::uint8_t>(result.num_positions);
        for (std::size_t i = 0; i < result.num_positions; ++i) {
            out.positions[i] = (result.positions[i] == LAST_CHAR)
                ? POS_LAST_CHAR
                : static_cast<std::uint8_t>(result.positions[i]);
        }
    }

    for (std::size_t s = 0; s < TableSize; ++s) {
        out.slot_to_key[s] = static_cast<std::uint8_t>(result.slot_to_key[s]);
    }

    for (std::size_t s = 0; s < TableSize; ++s) {
        std::size_t ki = result.slot_to_key[s];
        if (ki < N) {
            auto k = keys[ki];
            out.slot_key_len[s] = static_cast<std::uint8_t>(k.size());
            for (std::size_t c = 0; c < k.size(); ++c) { out.slot_key_bytes[s][c] = k[c]; }
        } else {
            out.slot_key_len[s] = 0; // empty slot: no length can match
        }
    }
    return out;
}

// --- SIMD runtime primitives ------------------------------------------------

// The runtime matchers read whole 16-byte blocks up to offset 63 (four blocks)
// for keys as long as the 63-character maximum. That read must stay within the
// buffer's trailing padding, so the padding has to exceed the largest offset we
// touch.
static_assert(SIMDJSON_PADDING > 63,
              "key_selector requires SIMDJSON_PADDING > 63 for its SIMD key reads");

// Scan for the terminating '"' starting at p. Returns its byte offset (= key
// length). Caller guarantees SIMDJSON_PADDING (== 64) bytes past the JSON buffer,
// so reading whole 16-byte blocks up to offset 63 is always safe.
template <std::size_t MaxKeyLen>
simdjson_really_inline std::size_t scan_key_length(const char* p) noexcept {
    // The closing quote of the longest key sits at most at offset MaxKeyLen, so we
    // scan as many 16-byte blocks as it takes to cover offsets 0..MaxKeyLen. The
    // cap (63) keeps every covered offset within the 64-byte padding guarantee, so
    // the SIMD and scalar builds agree.
    static_assert(MaxKeyLen <= 63, "MaxKeyLen must be <= 63 for current SIMD implementations");
    // [[maybe_unused]]: the scalar fallback (no NEON/SSE2/LSX) does not use it.
    [[maybe_unused]] constexpr std::size_t num_blocks = MaxKeyLen / 16 + 1;
#if SIMDJSON_KEY_SELECTOR_HAS_NEON
    for (std::size_t b = 0; b < num_blocks; ++b) {
        uint8x16_t v = vld1q_u8(reinterpret_cast<const uint8_t*>(p) + b * 16);
        uint8x16_t cmp = vceqq_u8(v, vdupq_n_u8('"'));
        uint64_t m = vget_lane_u64(
            vreinterpret_u64_u8(vshrn_n_u16(vreinterpretq_u16_u8(cmp), 4)), 0);
        if (simdjson_likely(m != 0)) { return b * 16 + (std::size_t(__builtin_ctzll(m)) >> 2); }
    }
    return MaxKeyLen + 1;
#elif SIMDJSON_KEY_SELECTOR_HAS_SSE2
    for (std::size_t b = 0; b < num_blocks; ++b) {
        __m128i v   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + b * 16));
        __m128i cmp = _mm_cmpeq_epi8(v, _mm_set1_epi8('"'));
        unsigned m  = static_cast<unsigned>(_mm_movemask_epi8(cmp));
        if (simdjson_likely(m != 0)) { return b * 16 + std::size_t(__builtin_ctz(m)); }
    }
    return MaxKeyLen + 1;
#elif SIMDJSON_KEY_SELECTOR_HAS_LSX
    for (std::size_t b = 0; b < num_blocks; ++b) {
        __m128i v   = __lsx_vld(reinterpret_cast<const void*>(p + b * 16), 0);
        __m128i cmp = __lsx_vseq_b(v, __lsx_vreplgr2vr_b('"'));
        // vmskltz_b gathers the per-byte sign bits (set where the byte equals '"')
        // into the low 16 bits of lane 0, the LSX equivalent of movemask.
        unsigned m  = static_cast<unsigned>(__lsx_vpickve2gr_w(__lsx_vmskltz_b(cmp), 0)) & 0xFFFFu;
        if (simdjson_likely(m != 0)) { return b * 16 + std::size_t(__builtin_ctz(m)); }
    }
    return MaxKeyLen + 1;
#else
    for (std::size_t i = 0; i <= MaxKeyLen; ++i)
        if (p[i] == '"') return i;
    return MaxKeyLen + 1;
#endif
}

// Byte-equal of p[0..len) against stored[0..len). stored is zero-padded past
// `len`. Input is read over 16, 32, 48, or 64 bytes (padded JSON buffer
// guaranteed; SIMDJSON_PADDING == 64).
template <std::size_t MaxKeyLen>
simdjson_really_inline bool compare_key_bytes(
    const char* p, const char* stored, std::size_t len) noexcept {
    // [[maybe_unused]]: only the NEON/SSE2/LSX branches read these; the scalar
    // fallback build (no SIMD) leaves them unused, which is an error under -Werror.
    [[maybe_unused]] alignas(16) static constexpr uint8_t idx16[16] =
        {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};
    if constexpr (MaxKeyLen <= 16) {
#if SIMDJSON_KEY_SELECTOR_HAS_NEON
        uint8x16_t vp = vld1q_u8(reinterpret_cast<const uint8_t*>(p));
        uint8x16_t vs = vld1q_u8(reinterpret_cast<const uint8_t*>(stored));
        uint8x16_t mask = vcltq_u8(vld1q_u8(idx16), vdupq_n_u8(static_cast<uint8_t>(len)));
        uint8x16_t diff = veorq_u8(vandq_u8(vp, mask), vs);
        // A 32-bit-lane horizontal max is enough to decide "all bytes equal"
        // (diff is zero iff every 32-bit word is zero) and is cheaper than a
        // byte-wide reduction.
        return vmaxvq_u32(vreinterpretq_u32_u8(diff)) == 0;
#elif SIMDJSON_KEY_SELECTOR_HAS_SSE2
        __m128i vp = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p));
        __m128i vs = _mm_loadu_si128(reinterpret_cast<const __m128i*>(stored));
        __m128i idx = _mm_load_si128(reinterpret_cast<const __m128i*>(idx16));
        __m128i mask = _mm_cmplt_epi8(idx, _mm_set1_epi8(static_cast<char>(len)));
        __m128i eq = _mm_cmpeq_epi8(_mm_and_si128(vp, mask), vs);
        return _mm_movemask_epi8(eq) == 0xFFFF;
#elif SIMDJSON_KEY_SELECTOR_HAS_LSX
        __m128i vp = __lsx_vld(reinterpret_cast<const void*>(p), 0);
        __m128i vs = __lsx_vld(reinterpret_cast<const void*>(stored), 0);
        __m128i idx = __lsx_vld(reinterpret_cast<const void*>(idx16), 0);
        __m128i mask = __lsx_vslt_b(idx, __lsx_vreplgr2vr_b(static_cast<int>(len)));
        __m128i eq = __lsx_vseq_b(__lsx_vand_v(vp, mask), vs);
        return (static_cast<unsigned>(__lsx_vpickve2gr_w(__lsx_vmskltz_b(eq), 0)) & 0xFFFFu) == 0xFFFFu;
#else
        for (std::size_t i = 0; i < len; ++i)
            if (p[i] != stored[i]) return false;
        return true;
#endif
    } else if constexpr (MaxKeyLen <= 32) {
        [[maybe_unused]] alignas(16) static constexpr uint8_t idx32_hi[16] =
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
        return vmaxvq_u32(vreinterpretq_u32_u8(vorrq_u8(d_lo, d_hi))) == 0;
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
#elif SIMDJSON_KEY_SELECTOR_HAS_LSX
        __m128i lenv = __lsx_vreplgr2vr_b(static_cast<int>(len));
        __m128i vp_lo = __lsx_vld(reinterpret_cast<const void*>(p), 0);
        __m128i vp_hi = __lsx_vld(reinterpret_cast<const void*>(p + 16), 0);
        __m128i vs_lo = __lsx_vld(reinterpret_cast<const void*>(stored), 0);
        __m128i vs_hi = __lsx_vld(reinterpret_cast<const void*>(stored + 16), 0);
        __m128i m_lo = __lsx_vslt_b(__lsx_vld(reinterpret_cast<const void*>(idx16), 0),    lenv);
        __m128i m_hi = __lsx_vslt_b(__lsx_vld(reinterpret_cast<const void*>(idx32_hi), 0), lenv);
        __m128i eq_lo = __lsx_vseq_b(__lsx_vand_v(vp_lo, m_lo), vs_lo);
        __m128i eq_hi = __lsx_vseq_b(__lsx_vand_v(vp_hi, m_hi), vs_hi);
        unsigned mlo = static_cast<unsigned>(__lsx_vpickve2gr_w(__lsx_vmskltz_b(eq_lo), 0)) & 0xFFFFu;
        unsigned mhi = static_cast<unsigned>(__lsx_vpickve2gr_w(__lsx_vmskltz_b(eq_hi), 0)) & 0xFFFFu;
        return (mlo & mhi) == 0xFFFFu;
#else
        for (std::size_t i = 0; i < len; ++i)
            if (p[i] != stored[i]) return false;
        return true;
#endif
    } else if constexpr (MaxKeyLen <= 64) {
        // 3 or 4 16-byte blocks (33..48 -> 3, 49..64 -> 4). stored is zero-padded
        // to exactly num_blocks*16 bytes (KEY_STRIDE), so neither load overruns it.
        // [[maybe_unused]]: the scalar fallback (no NEON/SSE2/LSX) does not use it.
        [[maybe_unused]] constexpr std::size_t num_blocks = (MaxKeyLen + 15) / 16;
#if SIMDJSON_KEY_SELECTOR_HAS_NEON
        uint8x16_t base = vld1q_u8(idx16);
        uint8x16_t lenv = vdupq_n_u8(static_cast<uint8_t>(len));
        uint8x16_t acc  = vdupq_n_u8(0);
        for (std::size_t b = 0; b < num_blocks; ++b) {
            uint8x16_t vp   = vld1q_u8(reinterpret_cast<const uint8_t*>(p) + b * 16);
            uint8x16_t vs   = vld1q_u8(reinterpret_cast<const uint8_t*>(stored) + b * 16);
            uint8x16_t idxv = vaddq_u8(base, vdupq_n_u8(static_cast<uint8_t>(b * 16)));
            uint8x16_t mask = vcltq_u8(idxv, lenv);
            acc = vorrq_u8(acc, veorq_u8(vandq_u8(vp, mask), vs));
        }
        return vmaxvq_u32(vreinterpretq_u32_u8(acc)) == 0;
#elif SIMDJSON_KEY_SELECTOR_HAS_SSE2
        __m128i base = _mm_load_si128(reinterpret_cast<const __m128i*>(idx16));
        __m128i lenv = _mm_set1_epi8(static_cast<char>(len));
        int eq = 0xFFFF;
        for (std::size_t b = 0; b < num_blocks; ++b) {
            __m128i vp   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p + b * 16));
            __m128i vs   = _mm_loadu_si128(reinterpret_cast<const __m128i*>(stored + b * 16));
            __m128i idxv = _mm_add_epi8(base, _mm_set1_epi8(static_cast<char>(b * 16)));
            __m128i mask = _mm_cmplt_epi8(idxv, lenv);
            eq &= _mm_movemask_epi8(_mm_cmpeq_epi8(_mm_and_si128(vp, mask), vs));
        }
        return eq == 0xFFFF;
#elif SIMDJSON_KEY_SELECTOR_HAS_LSX
        __m128i base = __lsx_vld(reinterpret_cast<const void*>(idx16), 0);
        __m128i lenv = __lsx_vreplgr2vr_b(static_cast<int>(len));
        unsigned acc = 0xFFFFu;
        for (std::size_t b = 0; b < num_blocks; ++b) {
            __m128i vp   = __lsx_vld(reinterpret_cast<const void*>(p + b * 16), 0);
            __m128i vs   = __lsx_vld(reinterpret_cast<const void*>(stored + b * 16), 0);
            __m128i idxv = __lsx_vadd_b(base, __lsx_vreplgr2vr_b(static_cast<int>(b * 16)));
            __m128i mask = __lsx_vslt_b(idxv, lenv);
            __m128i eq   = __lsx_vseq_b(__lsx_vand_v(vp, mask), vs);
            acc &= static_cast<unsigned>(__lsx_vpickve2gr_w(__lsx_vmskltz_b(eq), 0)) & 0xFFFFu;
        }
        return acc == 0xFFFFu;
#else
        for (std::size_t i = 0; i < len; ++i)
            if (p[i] != stored[i]) return false;
        return true;
#endif
    } else {
        for (std::size_t i = 0; i < len; ++i)
            if (p[i] != stored[i]) return false;
        return true;
    }
}

// --- Single 8-bit window fast path ------------------------------------------
//
// Many small key sets can be told apart by inspecting a *single* 8-bit window of
// the key bytes -- and that window need not be byte-aligned. Because every JSON
// key is terminated by a '"', the bytes at and before a key's length are well
// defined for any key at least that long: byte i is the key character when i is
// inside the key and the closing quote when i == len. So we read two bytes at a
// fixed offset, extract 8 consecutive bits at a fixed intra-byte shift, and if
// that value is distinct for every key, a 256-entry table maps it straight to a
// candidate key. The match then needs no hash and -- in the length-free overload
// -- no SIMD length scan: load two bytes, shift, mask, index the table, and
// confirm the candidate with one comparison.
//
// Allowing an *unaligned* window (a shift of 1..7) mixes bits from two adjacent
// bytes and discriminates key sets that no single aligned byte can. For example,
// the partial_tweets keys {created_at,id,text,in_reply_to_status_id,user,
// retweet_count,favorite_count} share a colliding byte at every aligned position
// 0,1,2, yet the 8 bits starting at bit offset 2 are unique across all seven.
// Simpler cases fall out as the shift==0 special case: {"id","screen_name"}
// splits on byte 0, {"jo","joe"} on the quote at byte 2.
//
// The window is confined to the first (shortest key length + 1) bytes so the
// two-byte read never crosses a key's closing quote into uncontrolled value
// bytes; that final byte is the shortest key's quote.
template <std::size_t N, std::size_t MaxKeyLen>
struct window_data {
    static constexpr std::size_t KEY_STRIDE = ((MaxKeyLen + 15) / 16) * 16;
    bool                                            ok{false};
    std::uint8_t                                    byte_offset{0}; // first byte of the 2-byte read
    std::uint8_t                                    shift{0};       // intra-byte bit shift (0..7)
    std::array<std::uint8_t, 256>                   window_to_key{}; // window byte -> key index, N if none
    std::array<std::uint8_t, N>                     key_len{};
    std::array<std::array<char, KEY_STRIDE>, N>     key_bytes{};     // zero-padded
};

// Byte seen at `idx` for key `i`: a key character when idx is inside the key, or
// the closing '"' when idx == len (callers keep idx <= every key's length).
template <std::size_t N>
constexpr unsigned window_byte_at(const std::array<std::string_view, N>& keys,
                                  std::size_t i, std::size_t idx) noexcept {
    if (idx < keys[i].size()) { return static_cast<unsigned char>(keys[i][idx]); }
    return static_cast<unsigned>('"');
}

// The 8-bit window value for key `i` at (byte_offset, shift): the little-endian
// pair (byte[off], byte[off+1]) shifted right by `shift` and truncated. Matches
// the runtime read exactly.
template <std::size_t N>
constexpr unsigned window_value(const std::array<std::string_view, N>& keys,
                                std::size_t i, std::size_t byte_offset, std::size_t shift) noexcept {
    unsigned lo = window_byte_at<N>(keys, i, byte_offset);
    unsigned hi = window_byte_at<N>(keys, i, byte_offset + 1);
    return ((lo | (hi << 8)) >> shift) & 0xFFu;
}

template <std::size_t N, std::size_t MaxKeyLen>
consteval window_data<N, MaxKeyLen>
compute_window(const std::array<std::string_view, N>& keys) {
    window_data<N, MaxKeyLen> out{};

    std::size_t min_len = keys[0].size();
    for (std::size_t i = 1; i < N; ++i) {
        if (keys[i].size() < min_len) { min_len = keys[i].size(); }
    }

    // Iterate windows nearest the front first (cheapest to read, smallest shift).
    for (std::size_t off = 0; off <= min_len; ++off) {
        for (std::size_t shift = 0; shift < 8; ++shift) {
            // The read touches byte off, and byte off+1 when shift != 0. Both must
            // stay within the safe region [0, min_len] (min_len is the shortest
            // key's quote index). off <= min_len is guaranteed by the loop bound.
            if (shift != 0 && off + 1 > min_len) { continue; }

            bool distinct = true;
            for (std::size_t i = 0; i < N && distinct; ++i) {
                for (std::size_t j = i + 1; j < N; ++j) {
                    if (window_value<N>(keys, i, off, shift) == window_value<N>(keys, j, off, shift)) {
                        distinct = false;
                        break;
                    }
                }
            }
            if (!distinct) { continue; }

            out.ok          = true;
            out.byte_offset = static_cast<std::uint8_t>(off);
            out.shift       = static_cast<std::uint8_t>(shift);
            for (std::size_t b = 0; b < 256; ++b) { out.window_to_key[b] = static_cast<std::uint8_t>(N); }
            for (std::size_t i = 0; i < N; ++i) {
                out.window_to_key[window_value<N>(keys, i, off, shift)] = static_cast<std::uint8_t>(i);
                out.key_len[i] = static_cast<std::uint8_t>(keys[i].size());
                for (std::size_t c = 0; c < keys[i].size(); ++c) { out.key_bytes[i][c] = keys[i][c]; }
            }
            return out;
        }
    }
    return out; // ok == false: no single 8-bit window distinguishes the keys
}

// Read the 8-bit window at (byte_offset, shift) from a padded key pointer.
simdjson_really_inline std::uint8_t read_window(const char* p, std::size_t byte_offset,
                                                std::size_t shift) noexcept {
    std::uint16_t w;
    // Two controlled bytes (within the shortest key + its quote, hence within the
    // padded buffer). memcpy is the portable little-endian unaligned load.
    std::memcpy(&w, p + byte_offset, sizeof(w));
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
    w = static_cast<std::uint16_t>((w >> 8) | (w << 8));
#endif
    return static_cast<std::uint8_t>((w >> shift) & 0xFFu);
}

// Verify a window candidate. The window table already mapped the key to the
// single possible index `ki` (< N); here we confirm it. Folding over 0..N-1
// turns the runtime `ki` into a compile-time index in the matching arm, so the
// candidate's length and bytes are constants for compare_key_bytes -- the same
// specialization the ordered find_field path gets from a CT-length
// unsafe_is_equal, and what keeps this path competitive. The closing-quote check
// (p[L] == '"') both confirms the key ends exactly at the candidate's length and
// rejects a wrong-length key, so this works whether or not the caller knew len.
template <std::size_t N, std::size_t MaxKeyLen, std::size_t... Is>
simdjson_really_inline std::size_t
match_window_candidate(const char* p, std::uint8_t ki,
                       const window_data<N, MaxKeyLen>& w,
                       std::index_sequence<Is...>) noexcept {
  std::size_t result = N;
  auto try_match = [&](auto Ic) {
    constexpr std::size_t i = decltype(Ic)::value;
    if (ki == i && p[w.key_len[i]] == '"' &&
        key_selector_detail::compare_key_bytes<MaxKeyLen>(
            p, w.key_bytes[i].data(), w.key_len[i])) {
      result = i;
    }
  };
  (try_match(std::integral_constant<std::size_t, Is>{}), ...);
  return result;
}

// --- describe() string helpers (constexpr; no std::to_string, which is not) ---

// Append the decimal form of v to s.
constexpr void append_uint(std::string& s, std::size_t v) {
    if (v == 0) { s.push_back('0'); return; }
    char buf[20];
    std::size_t n = 0;
    while (v > 0) { buf[n++] = static_cast<char>('0' + (v % 10)); v /= 10; }
    while (n > 0) { s.push_back(buf[--n]); }
}

// Append a byte as its decimal value, plus the printable character in quotes
// when it is in the printable ASCII range (e.g. "110 ('n')").
constexpr void append_byte(std::string& s, unsigned b) {
    append_uint(s, b);
    if (b >= 0x20 && b < 0x7f) {
        s += " ('";
        s.push_back(static_cast<char>(b));
        s += "')";
    }
}

} // namespace key_selector_detail

/**
 * Stateless, compile-time key selector.
 *
 * Usage:
 *   using sel_t = key_selector<"id", "text", "user">;
 *   std::size_t i = sel_t::match_raw(raw_key); // returns sel_t::size() on miss
 *
 * The perfect hash is built at compile time (gperf-style, with a
 * Hash-and-Displace fallback) and only flat tables survive to runtime. All
 * tables are static constexpr, so the lookup fully inlines.
 *
 * Limitations:
 *   - Each key must be at most 63 characters long (and no longer than
 *     SIMDJSON_PADDING). Longer keys trigger a compile-time error.
 *   - The number of keys should be moderate. The hard limit is 255 keys;
 *     compilation time grows with the number of keys, so prefer a few dozen at
 *     most per selector.
 *   - Keys must be distinct, non-empty, and free of backslash, double-quote and
 *     null bytes (matching is done against the raw, unescaped JSON key bytes).
 */
template <constevalutil::fixed_string... Keys>
struct key_selector {
    static constexpr std::size_t N = sizeof...(Keys);
    static_assert(N > 0,   "key_selector requires at least one key");
    static_assert(N <= 255,"key_selector supports at most 255 keys");

    static constexpr std::array<std::string_view, N> keys{ Keys.view()... };
    static constexpr std::size_t max_key_len = key_selector_detail::compute_max_key_len<N>(keys);
    static_assert(max_key_len <= SIMDJSON_PADDING,
                  "key longer than SIMDJSON_PADDING is not supported");
    // The SIMD key-length scan covers offsets 0..63 (four 16-byte blocks), which
    // stays within the 64-byte padding guarantee. A 64-character key's closing
    // quote would land at offset 64 and be missed on NEON/SSE2/LSX while still
    // matching in scalar builds, so cap at 63 to keep implementations in agreement.
    static_assert(max_key_len <= 63,
                  "key_selector keys must be at most 63 characters long");

    static constexpr auto result = key_selector_detail::compute_phf<N>(keys);
    static constexpr std::size_t table_size = result.table_size;

    static constexpr auto phf =
        key_selector_detail::build_phf_data<N, table_size, max_key_len>(keys, result);

    // Single 8-bit-window discriminator (when one exists). Detected at compile
    // time and selected with `if constexpr` below, so the hash path is compiled
    // out for key sets that qualify, and this is compiled out for those that do
    // not.
    static constexpr auto window =
        key_selector_detail::compute_window<N, max_key_len>(keys);

    static constexpr std::size_t size() noexcept { return N; }

    /**
     * Look up a JSON key whose length is already known. p must point at the first
     * key byte (just after the opening quote) in a padded simdjson buffer, and len
     * must be the number of raw key bytes (the distance to the closing quote).
     * Returns the selector index in [0, N) on match, or N on miss.
     *
     * Prefer this overload when the caller can obtain the key length cheaply (for
     * example, object::for_each derives it from the structural index rather than
     * re-scanning for the closing quote).
     */
    static simdjson_really_inline std::size_t match_raw(const char* p, std::size_t len) noexcept {
        if (len == 0 || len > max_key_len) { return N; }

        if constexpr (window.ok) {
            // One 8-bit window selects the only possible candidate key;
            // match_window_candidate confirms it (bytes + closing quote). p sits
            // in a padded buffer and the window stays within the shortest key +
            // quote, so the two-byte read is always in bounds. len is unused here
            // because the quote check already pins the key's end.
            std::uint8_t ki = window.window_to_key[
                key_selector_detail::read_window(p, window.byte_offset, window.shift)];
            if (ki >= N) { return N; }
            return key_selector_detail::match_window_candidate(
                p, ki, window, std::make_index_sequence<N>{});
        }

        std::size_t slot;
        if (phf.num_positions == key_selector_detail::HD_MODE) {
            // Hash-and-Displace: bucket displacement + per-key hash.
            std::string_view key(p, len);
            std::size_t bucket = key_selector_detail::hd_bucket_hash(key);
            std::size_t kh = (phf.hd_hash_variant == key_selector_detail::HD_HASH_2BYTE_FLAG)
                ? key_selector_detail::hd_key_hash_2(key)
                : key_selector_detail::hd_key_hash_4(key);
            slot = (phf.asso_values[0][bucket] + kh) & (table_size - 1);
        } else {
            // gperf: h = len + sum of asso_values over the selected positions.
            // positions / num_positions / asso_values are compile-time constants,
            // so this loop fully unrolls. The idx < len guard mirrors the
            // generator's char_at()-> 256 -> skip behavior for out-of-range
            // positions (required: arbitrary positions may exceed a key's length).
            std::size_t h = len;
            for (std::uint8_t i = 0; i < phf.num_positions; ++i) {
                std::uint8_t pos = phf.positions[i];
                std::size_t idx = (pos == key_selector_detail::POS_LAST_CHAR)
                                  ? (len - std::size_t{1})
                                  : static_cast<std::size_t>(pos);
                if (idx < len) {
                    h += phf.asso_values[i][static_cast<unsigned char>(p[idx])];
                }
            }
            slot = h & (table_size - 1);
        }

        std::uint8_t ki = phf.slot_to_key[slot];
        if (ki >= N) { return N; }
        if (phf.slot_key_len[slot] != len) { return N; }
        if (!key_selector_detail::compare_key_bytes<max_key_len>(
                p, phf.slot_key_bytes[slot].data(), len)) { return N; }
        return ki;
    }

    /**
     * Look up a JSON key. rjs must point just after an opening quote in a padded
     * simdjson buffer. Returns the selector index in [0, N) on match, or N on miss.
     * The key length is recovered with a SIMD scan for the closing quote; callers
     * that already know the length should use the (p, len) overload above.
     */
    static simdjson_really_inline std::size_t match_raw(raw_json_string rjs) noexcept {
        const char* p = rjs.raw();
        if constexpr (window.ok) {
            // One 8-bit window picks the candidate; verifying the candidate's
            // bytes and its closing '"' confirms the full key, so the length scan
            // is unnecessary. The window read is in bounds (padding), and the
            // candidate length is at most max_key_len.
            std::uint8_t ki = window.window_to_key[
                key_selector_detail::read_window(p, window.byte_offset, window.shift)];
            if (ki >= N) { return N; }
            return key_selector_detail::match_window_candidate(
                p, ki, window, std::make_index_sequence<N>{});
        }
        return match_raw(p, key_selector_detail::scan_key_length<max_key_len>(p));
    }

    /** Return the key text at selector index i (i in [0, N)). */
    static constexpr std::string_view key_at(std::size_t i) noexcept {
        return keys[i];
    }

    /**
     * Return a complete, human-readable, multi-line description of how this
     * selector classifies a key: which algorithm was selected at compile time
     * (single 8-bit window, gperf-style perfect hash, or hash-and-displace), the
     * exact bytes/positions it inspects, and the contents of the lookup tables
     * (which window bytes or hash slots map to which key). The text mirrors what
     * match_raw() does step by step.
     *
     * Everything it reports is derived from the compile-time tables, so describe()
     * is itself usable in a constant expression:
     *
     *   static_assert(!key_selector<"name", "city">::describe().empty());
     *
     * It allocates a std::string and is meant for documentation, debugging and
     * tests, not for any hot path.
     */
    static constexpr std::string describe() {
        std::string s;
        s += "key_selector: ";
        key_selector_detail::append_uint(s, N);
        s += " keys, max key length ";
        key_selector_detail::append_uint(s, max_key_len);
        s += "\nkeys:\n";
        for (std::size_t i = 0; i < N; ++i) {
            s += "  [";
            key_selector_detail::append_uint(s, i);
            s += "] \"";
            s += keys[i];
            s += "\" (length ";
            key_selector_detail::append_uint(s, keys[i].size());
            s += ")\n";
        }
        if constexpr (window.ok) {
            // Mirrors the window fast path of match_raw().
            s += "algorithm: single 8-bit window\n";
            s += "  step 1: read 2 bytes at offset ";
            key_selector_detail::append_uint(s, window.byte_offset);
            s += ", interpret them as a little-endian 16-bit value, shift right by ";
            key_selector_detail::append_uint(s, window.shift);
            s += " bits, and keep the low 8 bits\n";
            s += "  step 2: map that byte through a 256-entry table to a key index (";
            key_selector_detail::append_uint(s, N);
            s += " means no match):\n";
            for (std::size_t b = 0; b < 256; ++b) {
                if (window.window_to_key[b] < N) {
                    s += "    byte ";
                    key_selector_detail::append_byte(s, static_cast<unsigned>(b));
                    s += " -> key ";
                    key_selector_detail::append_uint(s, window.window_to_key[b]);
                    s += "\n";
                }
            }
            s += "  step 3: confirm the candidate by checking the closing quote sits at the key's length and comparing the key bytes\n";
        } else {
            // Mirrors the perfect-hash path of match_raw().
            if constexpr (phf.num_positions == key_selector_detail::HD_MODE) {
                s += "algorithm: hash-and-displace perfect hash\n";
                s += "  step 1: bucket = (first_byte + last_byte*3 + length*17) mod 256\n";
                s += "  step 2: keyhash = base-31 rolling hash of the length and the first ";
                key_selector_detail::append_uint(s, phf.hd_hash_variant);
                s += " bytes\n";
                s += "  step 3: slot = (displacement[bucket] + keyhash) mod ";
                key_selector_detail::append_uint(s, table_size);
                s += "\n  step 4: slot_to_key[slot] gives the candidate key index\n";
                s += "  per-key derivation:\n";
                for (std::size_t i = 0; i < N; ++i) {
                    std::string_view k = keys[i];
                    std::size_t bucket = key_selector_detail::hd_bucket_hash(k);
                    std::size_t kh = (phf.hd_hash_variant == key_selector_detail::HD_HASH_2BYTE_FLAG)
                        ? key_selector_detail::hd_key_hash_2(k)
                        : key_selector_detail::hd_key_hash_4(k);
                    std::size_t slot = (phf.asso_values[0][bucket] + kh) & (table_size - 1);
                    s += "    \"";
                    s += k;
                    s += "\": bucket=";
                    key_selector_detail::append_uint(s, bucket);
                    s += " displacement=";
                    key_selector_detail::append_uint(s, phf.asso_values[0][bucket]);
                    s += " keyhash=";
                    key_selector_detail::append_uint(s, kh);
                    s += " slot=";
                    key_selector_detail::append_uint(s, slot);
                    s += "\n";
                }
            } else {
                s += "algorithm: gperf-style perfect hash over ";
                key_selector_detail::append_uint(s, phf.num_positions);
                s += " character position(s)\n";
                s += "  step 1: h = key length\n";
                s += "  step 2: for each position below, add its association value for the key byte there (a position past the key's length contributes 0):\n";
                for (std::size_t i = 0; i < phf.num_positions; ++i) {
                    s += "    position ";
                    if (phf.positions[i] == key_selector_detail::POS_LAST_CHAR) {
                        s += "last character";
                    } else {
                        s += "byte index ";
                        key_selector_detail::append_uint(s, phf.positions[i]);
                    }
                    s += "\n";
                }
                s += "  step 3: slot = h mod ";
                key_selector_detail::append_uint(s, table_size);
                s += " (a power of two, applied as a bitmask)\n";
                s += "  step 4: slot_to_key[slot] gives the candidate key index\n";
                s += "  per-key derivation:\n";
                for (std::size_t i = 0; i < N; ++i) {
                    std::string_view k = keys[i];
                    std::size_t h = k.size();
                    for (std::size_t pi = 0; pi < phf.num_positions; ++pi) {
                        std::size_t pos = phf.positions[pi];
                        std::size_t idx = (pos == key_selector_detail::POS_LAST_CHAR)
                                          ? (k.size() - 1) : pos;
                        if (idx < k.size()) {
                            h += phf.asso_values[pi][static_cast<unsigned char>(k[idx])];
                        }
                    }
                    std::size_t slot = h & (table_size - 1);
                    s += "    \"";
                    s += k;
                    s += "\": h=";
                    key_selector_detail::append_uint(s, h);
                    s += " slot=";
                    key_selector_detail::append_uint(s, slot);
                    s += "\n";
                }
            }
            s += "  occupied slots (slot -> key):\n";
            for (std::size_t slot = 0; slot < table_size; ++slot) {
                if (phf.slot_to_key[slot] < N) {
                    s += "    slot ";
                    key_selector_detail::append_uint(s, slot);
                    s += " -> key ";
                    key_selector_detail::append_uint(s, phf.slot_to_key[slot]);
                    s += " (\"";
                    s += keys[phf.slot_to_key[slot]];
                    s += "\", length ";
                    key_selector_detail::append_uint(s, phf.slot_key_len[slot]);
                    s += ")\n";
                }
            }
            s += "  confirm the candidate by checking the key length matches and comparing the key bytes\n";
        }
        return s;
    }
};

namespace key_selector_detail {
template <typename> struct is_key_selector : std::false_type {};
template <constevalutil::fixed_string... Keys>
struct is_key_selector<key_selector<Keys...>> : std::true_type {};
} // namespace key_selector_detail

/**
 * Matches any instantiation of key_selector<Keys...>. Used to constrain
 * object::for_each so that passing a non-selector type yields a clear
 * constraint error rather than a cascade of failures inside for_each.
 */
template <typename T>
concept key_selector_type = key_selector_detail::is_key_selector<T>::value;

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_CONCEPTS

#endif // SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H
