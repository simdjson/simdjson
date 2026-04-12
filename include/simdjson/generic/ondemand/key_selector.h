#ifndef SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H
#define SIMDJSON_GENERIC_ONDEMAND_KEY_SELECTOR_H

#include "simdjson/base.h"
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

    // Perfect hash table data
    std::array<std::array<std::uint8_t, 256>, 16> asso_values_{};
    std::uint8_t num_positions_{};
    std::array<std::uint8_t, 16> positions_{};
    std::array<std::uint8_t, 100> slot_to_key_{}; // Max table size
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

    // Simple perfect hash generation using polynomial rolling hash
    constexpr void generate_hash_table(const std::array<std::string_view, N>& keys) {
        // Use polynomial hash: hash = sum(key[i] * 31^(len-1-i)) % table_size
        const std::size_t prime = 31;

        // Find minimum table size that works
        std::size_t table_size = N;
        bool success = false;
        while (!success && table_size <= 100) { // Max table size
            success = true;
            std::array<std::uint8_t, 100> used_slots{};
            std::fill(used_slots.begin(), used_slots.begin() + table_size, 0);
            // Initialize slot_to_key_ to invalid values
            std::fill(slot_to_key_.begin(), slot_to_key_.begin() + table_size, static_cast<std::uint8_t>(N));

            for (std::size_t i = 0; i < N && success; ++i) {
                std::size_t hash = 0;
                std::size_t power = 1;
                for (std::size_t j = keys[i].size(); j > 0; --j) {
                    unsigned char ch = static_cast<unsigned char>(keys[i][j-1]);
                    hash = (hash + ch * power) % table_size;
                    power = (power * prime) % table_size;
                }
                std::size_t slot = hash;

                if (used_slots[slot]) {
                    success = false;
                } else {
                    used_slots[slot] = 1;
                    slot_to_key_[slot] = static_cast<std::uint8_t>(i);
                }
            }

            if (!success) {
                table_size++;
            }
        }

        table_size_ = table_size;

        // Build key_to_slot mapping
        for (std::size_t slot = 0; slot < table_size_; ++slot) {
            std::uint8_t key_idx = slot_to_key_[slot];
            if (key_idx < N) {
                key_to_slot_[key_idx] = static_cast<std::uint8_t>(slot);
            }
        }
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

    [[nodiscard]] constexpr std::size_t compute_hash(std::string_view key) const noexcept {
        const std::size_t prime = 31;
        std::size_t hash = 0;
        std::size_t power = 1;
        for (std::size_t j = key.size(); j > 0; --j) {
            unsigned char ch = static_cast<unsigned char>(key[j-1]);
            hash = (hash + ch * power) % table_size_;
            power = (power * prime) % table_size_;
        }
        return hash;
    }

    [[nodiscard]] constexpr bool contains(std::string_view key) const noexcept {
        std::size_t slot = compute_hash(key);
        if (slot >= table_size_) return false;

        std::uint8_t key_idx = slot_to_key_[slot];
        if (key_idx >= N) return false;

        // Compare key
        if (key_lengths_[key_idx] != key.size()) return false;
        return std::equal(key.begin(), key.end(), key_data_[key_idx].begin());
    }

    [[nodiscard]] constexpr std::size_t index_of(std::string_view key) const noexcept {
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