#ifndef SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H
#define SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H

#include "simdjson/base.h"
#include "simdjson/common_defs.h"
#include <array>
#include <string_view>
#include <cstddef>
#include <cstdint>
#include <cstring>

#if SIMDJSON_SUPPORTS_CONCEPTS

namespace simdjson {
namespace SIMDJSON_IMPLEMENTATION {
namespace ondemand {



// Forward declaration
class object;

/**
 * A compile-time key selector for efficient JSON object field lookup.
 * Uses perfect hashing (gperf-style) to map keys to identifiers.
 */
template <std::size_t N>
class key_selector {
    static_assert(N > 0, "key_selector requires at least one key");
    static_assert(N <= 100, "key_selector supports at most 100 keys");

    // Perfect hash table data (gperf-style)
    static constexpr std::size_t MAX_POSITIONS = 16;
    static constexpr std::size_t POS_LAST_CHAR = std::size_t(-1);
    static constexpr std::size_t MAX_TABLE_SIZE = 256; // Power of 2, fits in uint8_t

    std::array<std::array<std::uint8_t, 256>, MAX_POSITIONS> asso_values_{};
    std::uint8_t num_positions_{};
    std::array<std::size_t, MAX_POSITIONS> positions_{};
    std::array<std::uint8_t, MAX_TABLE_SIZE> slot_to_key_{};
    std::array<std::uint8_t, N> key_to_slot_{};
    std::array<std::array<char, 64>, N> key_data_{};
    std::array<std::uint8_t, N> key_lengths_{};
    std::size_t table_size_{};

public:
    // Validate keys at compile time
    constexpr void validate_keys(const std::array<std::string_view, N>& keys) {
        for (std::size_t i = 0; i < N; ++i) {
            auto key = keys[i];
            if (key.empty()) {
                throw "Empty keys are not allowed in key_selector";
            }
            if (key.size() > SIMDJSON_PADDING) {
                throw "Key length exceeds SIMDJSON_PADDING (64 bytes)";
            }
            for (char c : key) {
                if (c == '\\') {
                    throw "Escape characters (\\) are not allowed in key_selector keys";
                }
                if (c == '\0') {
                    throw "Null characters are not allowed in key_selector keys";
                }
            }
        }
    }

    // Gperf-style perfect hash generation using partition-based algorithm
    constexpr void generate_hash_table(const std::array<std::string_view, N>& keys) {
        // Try power-of-two table sizes starting from next_power_of_2(N)
        constexpr std::size_t START_M = next_power_of_2(N);
        if constexpr (START_M <= MAX_TABLE_SIZE) {
            if (try_compute_phf<START_M>(keys)) return;
            if constexpr (START_M * 2 <= MAX_TABLE_SIZE) {
                if (try_compute_phf<START_M * 2>(keys)) return;
                if constexpr (START_M * 4 <= MAX_TABLE_SIZE) {
                    if (try_compute_phf<START_M * 4>(keys)) return;
                    if constexpr (START_M * 8 <= MAX_TABLE_SIZE) {
                        if (try_compute_phf<START_M * 8>(keys)) return;
                    }
                }
            }
        }

        // Fallback: linear table
        table_size_ = N;
        num_positions_ = 0;
        std::fill(slot_to_key_.begin(), slot_to_key_.begin() + MAX_TABLE_SIZE, static_cast<std::uint8_t>(N));
        for (std::size_t i = 0; i < N; ++i) {
            slot_to_key_[i] = static_cast<std::uint8_t>(i);
            key_to_slot_[i] = static_cast<std::uint8_t>(i);
        }
    }

private:
    // Helper functions for gperf algorithm
    static constexpr std::size_t next_power_of_2(std::size_t n) {
        if (n == 0) return 1;
        std::size_t p = 1;
        while (p < n) p <<= 1;
        return p;
    }

    static constexpr std::size_t char_at(std::string_view key, std::size_t pos) {
        if (pos == POS_LAST_CHAR) {
            return key.empty() ? 256 : static_cast<unsigned char>(key.back());
        }
        return (pos < key.size()) ? static_cast<unsigned char>(key[pos]) : 256;
    }

    template <std::size_t M>
    constexpr bool try_compute_phf(const std::array<std::string_view, N>& keys) {
        // Initialize
        std::array<std::array<std::size_t, 256>, MAX_POSITIONS> asso{};
        std::size_t npos = 0;
        std::array<std::size_t, MAX_POSITIONS> pos{};
        std::array<std::size_t, M> s2k{};

        // Try to generate gperf
        if (try_generate_gperf<M>(keys, asso, npos, pos, s2k)) {
            table_size_ = M;
            num_positions_ = static_cast<std::uint8_t>(npos);
            for (std::size_t p = 0; p < MAX_POSITIONS; ++p) {
                positions_[p] = pos[p];
                for (std::size_t c = 0; c < 256; ++c) {
                    asso_values_[p][c] = static_cast<std::uint8_t>(asso[p][c]);
                }
            }
            for (std::size_t i = 0; i < M; ++i) {
                slot_to_key_[i] = static_cast<std::uint8_t>(s2k[i]);
            }
            // Fill remaining slots with sentinel
            for (std::size_t i = M; i < MAX_TABLE_SIZE; ++i) {
                slot_to_key_[i] = static_cast<std::uint8_t>(N);
            }

            // Build key_to_slot mapping
            for (std::size_t i = 0; i < N; ++i) {
                key_to_slot_[i] = static_cast<std::uint8_t>(N); // Initialize
            }
            for (std::size_t slot = 0; slot < M; ++slot) {
                std::size_t key_idx = s2k[slot];
                if (key_idx < N) {
                    key_to_slot_[key_idx] = static_cast<std::uint8_t>(slot);
                }
            }
            return true;
        }
        return false;
    }

    template <std::size_t M>
    static constexpr bool try_generate_gperf(
        const std::array<std::string_view, N>& keys,
        std::array<std::array<std::size_t, 256>, MAX_POSITIONS>& asso_values,
        std::size_t& num_positions,
        std::array<std::size_t, MAX_POSITIONS>& positions,
        std::array<std::size_t, M>& slot_to_key)
    {
        // Initialize
        for (std::size_t p = 0; p < MAX_POSITIONS; ++p) {
            for (std::size_t c = 0; c < 256; ++c) {
                asso_values[p][c] = 0;
            }
        }
        for (std::size_t i = 0; i < M; ++i) {
            slot_to_key[i] = N;
        }

        // Try length-only hashing first
        bool success = true;
        for (std::size_t i = 0; i < N && success; ++i) {
            std::size_t slot = keys[i].size() % M;
            if (slot_to_key[slot] != N) {
                success = false;
            } else {
                slot_to_key[slot] = i;
            }
        }

        if (success) {
            num_positions = 0;
            return true;
        }

        // Try with position 0
        positions[0] = 0;
        num_positions = 1;

        // Find a working assignment of asso_values for position 0
        // Use a simple approach: try different offsets
        for (std::size_t offset = 0; offset < M; ++offset) {
            // Reset
            for (std::size_t i = 0; i < M; ++i) {
                slot_to_key[i] = N;
            }

            // Assign asso_values based on offset
            for (std::size_t c = 0; c < 256; ++c) {
                asso_values[0][c] = (c + offset) % M;
            }

            success = true;
            for (std::size_t i = 0; i < N && success; ++i) {
                std::size_t h = keys[i].size();
                std::size_t ch = char_at(keys[i], 0);
                if (ch < 256) h += asso_values[0][ch];
                std::size_t slot = h % M;

                if (slot_to_key[slot] != N) {
                    success = false;
                } else {
                    slot_to_key[slot] = i;
                }
            }

            if (success) {
                return true;
            }
        }

        // Try with positions {0, last_char}
        if (N <= 50) { // Only for smaller N to avoid complexity
            positions[0] = 0;
            positions[1] = POS_LAST_CHAR;
            num_positions = 2;

            for (std::size_t offset1 = 0; offset1 < 4 && !success; ++offset1) {
                for (std::size_t offset2 = 0; offset2 < 4 && !success; ++offset2) {
                    // Reset
                    for (std::size_t i = 0; i < M; ++i) {
                        slot_to_key[i] = N;
                    }

                    // Assign asso_values
                    for (std::size_t c = 0; c < 256; ++c) {
                        asso_values[0][c] = (c + offset1) % M;
                        asso_values[1][c] = (c + offset2) % M;
                    }

                    success = true;
                    for (std::size_t i = 0; i < N && success; ++i) {
                        std::size_t h = keys[i].size();
                        std::size_t ch1 = char_at(keys[i], 0);
                        if (ch1 < 256) h += asso_values[0][ch1];
                        std::size_t ch2 = char_at(keys[i], POS_LAST_CHAR);
                        if (ch2 < 256) h += asso_values[1][ch2];
                        std::size_t slot = h % M;

                        if (slot_to_key[slot] != N) {
                            success = false;
                        } else {
                            slot_to_key[slot] = i;
                        }
                    }

                    if (success) {
                        return true;
                    }
                }
            }
        }

        return false;
    }



public:
    constexpr key_selector(const std::array<std::string_view, N>& keys) {
        validate_keys(keys);

        // Store key data
        for (std::size_t i = 0; i < N; ++i) {
            key_lengths_[i] = static_cast<std::uint8_t>(keys[i].size());
            std::copy(keys[i].begin(), keys[i].end(), key_data_[i].begin());
        }

        generate_hash_table(keys);
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return N; }

    [[nodiscard]] constexpr simdjson_really_inline std::size_t compute_hash(std::string_view key) const noexcept {
        std::size_t h = key.size();
        const char* kp = key.data();
        for (std::uint8_t i = 0; i < num_positions_; ++i) {
            std::size_t pos = positions_[i];
            std::size_t ch;
            if (pos == POS_LAST_CHAR) {
                ch = static_cast<unsigned char>(key.back());
            } else {
                ch = static_cast<unsigned char>(kp[pos]);
            }
            h += asso_values_[i][ch];
        }
        return h & (table_size_ - 1);
    }

    [[nodiscard]] constexpr simdjson_really_inline  bool contains(std::string_view key) const noexcept {
        std::size_t slot = compute_hash(key);
        if (slot >= table_size_) return false;

        std::uint8_t key_idx = slot_to_key_[slot];
        if (key_idx >= N) return false;

        // Compare key
        if (key_lengths_[key_idx] != key.size()) return false;
        return std::equal(key.begin(), key.end(), key_data_[key_idx].begin());
    }

    [[nodiscard]] constexpr simdjson_really_inline std::size_t index_of(std::string_view key) const noexcept {
        std::size_t slot = compute_hash(key);
        if (slot >= table_size_) return N; // Invalid index

        std::uint8_t key_idx = slot_to_key_[slot];
        if (key_idx >= N) return N;

        // Compare key
        if (key_lengths_[key_idx] != key.size()) return N;
        if (!std::equal(key.begin(), key.end(), key_data_[key_idx].begin())) return N;

        return key_idx;
    }

    // Accessors for key data (used by object::find_field)
    [[nodiscard]] constexpr std::string_view get_key(std::size_t index) const noexcept {
        if (index >= N) return {};
        return std::string_view(key_data_[index].data(), key_lengths_[index]);
    }
};

} // namespace ondemand
} // namespace SIMDJSON_IMPLEMENTATION
} // namespace simdjson

#endif // SIMDJSON_SUPPORTS_CONCEPTS

#endif // SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H